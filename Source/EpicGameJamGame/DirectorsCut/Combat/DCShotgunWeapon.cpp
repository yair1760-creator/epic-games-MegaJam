// Director's Cut - Beat 3-5 shotgun.

#include "DirectorsCut/Combat/DCShotgunWeapon.h"
#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Combat/DCHitFeedbackComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraSystem.h"
#include "ShooterProjectile.h"
#include "ShooterWeaponHolder.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ADCShotgunWeapon::ADCShotgunWeapon()
{
	// Shotgun tuning: 8 shells per mag, punchy semi-auto blast, 8-pellet spread
	MagazineSize = 8;
	ReloadTime = 1.5f;
	AimVariance = 0.0f;
	FiringRecoil = -0.45f;
	MuzzleSocketName = FName("Muzzle");
	MuzzleOffset = 15.0f;
	bFullAuto = false;
	RefireRate = 0.55f;
	ShotLoudness = 1.5f;
	ShotNoiseRange = 600.0f;

	static ConstructorHelpers::FClassFinder<AShooterProjectile> BulletBP(TEXT("/Game/Variant_Shooter/Blueprints/Pickups/Projectiles/BP_ShooterProjectile_Bullet"));
	if (BulletBP.Succeeded())
	{
		ProjectileClass = BulletBP.Class;
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ShotgunMesh(TEXT("/Game/Weapons/GrenadeLauncher/Meshes/SKM_GrenadeLauncher.SKM_GrenadeLauncher"));
	if (ShotgunMesh.Succeeded())
	{
		GetFirstPersonMesh()->SetSkeletalMesh(ShotgunMesh.Object);
		GetThirdPersonMesh()->SetSkeletalMesh(ShotgunMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> FPAnimBP(TEXT("/Game/Variant_Shooter/Anims/ABP_FP_Weapon"));
	if (FPAnimBP.Succeeded())
	{
		FirstPersonAnimInstanceClass = FPAnimBP.Class;
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> TPAnimBP(TEXT("/Game/Variant_Shooter/Anims/ABP_TP_Rifle"));
	if (TPAnimBP.Succeeded())
	{
		ThirdPersonAnimInstanceClass = TPAnimBP.Class;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ShootMontage(TEXT("/Game/Variant_Shooter/Anims/FP_Rifle_Shoot_Montage.FP_Rifle_Shoot_Montage"));
	if (ShootMontage.Succeeded())
	{
		FiringMontage = ShootMontage.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> BlastSFX(TEXT("/Game/Weapons/GrenadeLauncher/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02"));
	if (BlastSFX.Succeeded())
	{
		FireSound = BlastSFX.Object;
	}
}

void ADCShotgunWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Allow firing immediately on equip without waiting RefireRate seconds after spawn
	TimeOfLastShot = -RefireRate - 1.0f;
}

void ADCShotgunWeapon::FireProjectile(const FVector& TargetLocation)
{
	if (!WeaponOwner || !ProjectileClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector MuzzleLoc = FVector::ZeroVector;
	if (USkeletalMeshComponent* FPMesh = GetFirstPersonMesh(); FPMesh && FPMesh->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleLoc = FPMesh->GetSocketLocation(MuzzleSocketName);
	}
	else if (PawnOwner)
	{
		FRotator EyeRot;
		PawnOwner->GetActorEyesViewPoint(MuzzleLoc, EyeRot);
		MuzzleLoc += EyeRot.Vector() * 40.0f;
	}

	FVector AimDir = (TargetLocation - MuzzleLoc).GetSafeNormal();
	if (AimDir.IsNearlyZero() && PawnOwner)
	{
		AimDir = PawnOwner->GetControlRotation().Vector();
	}

	const float SpreadRad = FMath::DegreesToRadians(FMath::Max(SpreadAngleDeg, 0.0f));
	const int32 NumPellets = FMath::Max(PelletCount, 1);

	TArray<AShooterProjectile*> SpawnedPellets;
	TArray<FTransform> SpawnTransforms;
	SpawnedPellets.Reserve(NumPellets);
	SpawnTransforms.Reserve(NumPellets);

	for (int32 i = 0; i < NumPellets; ++i)
	{
		const FVector PelletDir = (SpreadRad > 0.0f) ? FMath::VRandCone(AimDir, SpreadRad) : AimDir;
		const FVector SpawnLoc = MuzzleLoc + PelletDir * MuzzleOffset;
		const FTransform SpawnTransform(PelletDir.Rotation(), SpawnLoc, FVector(PelletScale));

		AShooterProjectile* Pellet = World->SpawnActorDeferred<AShooterProjectile>(
			ProjectileClass,
			SpawnTransform,
			GetOwner(),
			PawnOwner,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if (Pellet)
		{
			Pellet->SetNoiseTag(NoiseOwnerTag);
			SpawnedPellets.Add(Pellet);
			SpawnTransforms.Add(SpawnTransform);
		}
	}

	// Ensure pellets in the same volley never collide with each other out of the barrel
	for (int32 i = 0; i < SpawnedPellets.Num(); ++i)
	{
		UPrimitiveComponent* CompA = Cast<UPrimitiveComponent>(SpawnedPellets[i]->GetRootComponent());
		if (!CompA)
		{
			CompA = SpawnedPellets[i]->FindComponentByClass<USphereComponent>();
		}
		if (CompA)
		{
			for (int32 j = 0; j < SpawnedPellets.Num(); ++j)
			{
				if (i != j)
				{
					CompA->IgnoreActorWhenMoving(SpawnedPellets[j], true);
				}
			}
		}
	}

	for (int32 i = 0; i < SpawnedPellets.Num(); ++i)
	{
		SpawnedPellets[i]->FinishSpawning(SpawnTransforms[i]);
	}

	WeaponOwner->PlayFiringMontage(FiringMontage);
	WeaponOwner->AddWeaponRecoil(FiringRecoil);

	if (FireShakeDegrees > 0.0f && PawnOwner)
	{
		if (APlayerController* PC = Cast<APlayerController>(PawnOwner->GetController()))
		{
			if (PC->PlayerCameraManager)
			{
				UDCHitShakeModifier* Shake = Cast<UDCHitShakeModifier>(PC->PlayerCameraManager->FindCameraModifierByClass(UDCHitShakeModifier::StaticClass()));
				if (!Shake)
				{
					Shake = Cast<UDCHitShakeModifier>(PC->PlayerCameraManager->AddNewCameraModifier(UDCHitShakeModifier::StaticClass()));
				}
				if (Shake)
				{
					Shake->Trigger(FireShakeDegrees, 0.14f);
				}
			}
		}
	}

	UDCFXLibrary::PlayWorldSound(this, FireSound, MuzzleLoc, 1.0f, 0.05f, 0.02f, FireSoundPitch);
	UDCFXLibrary::SpawnWorldFX(this, FireFX, MuzzleLoc, AimDir.Rotation());

	--CurrentBullets;

	if (CurrentBullets <= 0 && !IsOwnedByPlayer())
	{
		CurrentBullets = MagazineSize;
	}

	WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
}
