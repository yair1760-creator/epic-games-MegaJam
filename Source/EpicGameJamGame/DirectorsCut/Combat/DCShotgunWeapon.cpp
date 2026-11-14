// Director's Cut - Beat 3-5 shotgun.

#include "DirectorsCut/Combat/DCShotgunWeapon.h"
#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Combat/DCHitFeedbackComponent.h"
#include "DirectorsCut/Combat/DCWeaponSpriteWidget.h"
#include "EpicGameJamGameCharacter.h"
#include "Engine/Texture2D.h"
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

	// 2D first-person frames (Art/Player/Shotgun)
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameIdle(TEXT("/Game/DirectorsCut/Combat/Shotgun/Sprites/T_Shotgun_Idle.T_Shotgun_Idle"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameFlash(TEXT("/Game/DirectorsCut/Combat/Shotgun/Sprites/T_Shotgun_Flash.T_Shotgun_Flash"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameRecoil(TEXT("/Game/DirectorsCut/Combat/Shotgun/Sprites/T_Shotgun_Recoil.T_Shotgun_Recoil"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameRecovery(TEXT("/Game/DirectorsCut/Combat/Shotgun/Sprites/T_Shotgun_Recovery.T_Shotgun_Recovery"));
	if (FrameIdle.Succeeded())     SpriteIdle = FrameIdle.Object;
	if (FrameFlash.Succeeded())    SpriteFlash = FrameFlash.Object;
	if (FrameRecoil.Succeeded())   SpriteRecoil = FrameRecoil.Object;
	if (FrameRecovery.Succeeded()) SpriteRecovery = FrameRecovery.Object;
}

void ADCShotgunWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Allow firing immediately on equip without waiting RefireRate seconds after spawn
	TimeOfLastShot = -RefireRate - 1.0f;

	if (bUseSpriteView)
	{
		// the sequence plays once per accepted fire input: never accept the next shot before it has finished
		RefireRate = FMath::Max(RefireRate, GetShotAnimDuration());
		TimeOfLastShot = -RefireRate - 1.0f;

		// the 2D frames replace the 3D gun in first person (its muzzle socket still gives the pellets a start point)
		if (USkeletalMeshComponent* FPMesh = GetFirstPersonMesh())
		{
			FPMesh->SetHiddenInGame(true, true);
		}
	}
}

void ADCShotgunWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	RemoveSpriteWidget();
	if (bUseSpriteView && !IsHidden())
	{
		SetOwnerArmsHidden(false);
	}
	Super::EndPlay(EndPlayReason);
}

void ADCShotgunWeapon::RemoveSpriteWidget()
{
	if (SpriteWidget)
	{
		SpriteWidget->RemoveFromParent();
		SpriteWidget = nullptr;
	}
}

void ADCShotgunWeapon::SetOwnerArmsHidden(bool bArmsHidden)
{
	if (AEpicGameJamGameCharacter* Char = Cast<AEpicGameJamGameCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Arms = Char->GetFirstPersonMesh())
		{
			Arms->SetHiddenInGame(bArmsHidden, false);
		}
	}
}

void ADCShotgunWeapon::SetActorHiddenInGame(bool bNewHidden)
{
	Super::SetActorHiddenInGame(bNewHidden);

	if (!bUseSpriteView)
	{
		return;
	}

	// the drawn hands replace the template arms while the shotgun is out
	SetOwnerArmsHidden(!bNewHidden);

	// the 2D hands go away with the weapon (holstered, owner died); a holstered gun is not mid-shot
	if (bNewHidden)
	{
		ShotTime = -1.0f;
		CurrentPose = TEXT("Idle");
		if (SpriteWidget)
		{
			SpriteWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ADCShotgunWeapon::UpdateSpriteView(float DeltaSeconds)
{
	APawn* Pawn = PawnOwner.Get();
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const bool bShow = bUseSpriteView && PC && PC->IsLocalController() && !IsHidden();

	if (!bShow)
	{
		if (SpriteWidget)
		{
			SpriteWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (!SpriteWidget)
	{
		SpriteWidget = CreateWidget<UDCWeaponSpriteWidget>(PC, UDCWeaponSpriteWidget::StaticClass());
		if (!SpriteWidget)
		{
			return;
		}
		SpriteWidget->AddToViewport(-10);   // under the HUD (health bar, quest, subtitles)
	}
	SpriteWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	// the weapon that was out before (the sword) shows the template arms again when it is put away,
	// and that can happen after this one was taken out: keep them hidden while the drawn hands are up
	SetOwnerArmsHidden(true);

	// pose from the 12 fps tick grid: hard cuts, each drawing held for whole ticks
	UTexture2D* Frame = SpriteIdle;
	FName Pose = TEXT("Idle");
	if (ShotTime >= 0.0f)
	{
		const int32 RecoilStart = FlashTicks;
		const int32 RecoveryStart = FlashTicks + RecoilTicks;
		const int32 End = RecoveryStart + RecoveryTicks;
		int32 AnimTick = FMath::FloorToInt(ShotTime * FMath::Max(AnimFPS, 1.0f));

		// a slow frame may jump straight past the one-tick peak recoil: show it anyway, it is the snap of the shot
		if (AnimTick >= RecoveryStart && !bRecoilShown)
		{
			AnimTick = RecoilStart;
		}

		if (AnimTick < RecoilStart)         { Frame = SpriteFlash;    Pose = TEXT("Flash"); }
		else if (AnimTick < RecoveryStart)  { Frame = SpriteRecoil;   Pose = TEXT("Recoil"); bRecoilShown = true; }
		else if (AnimTick < End)            { Frame = SpriteRecovery; Pose = TEXT("Recovery"); }
	}
	CurrentPose = Pose;
	SpriteWidget->SetFrame(Frame ? Frame : SpriteIdle.Get());

	// walk bob / idle breathing while idle; the shot poses keep the exact drawn placement
	BobTime += DeltaSeconds;
	const float Run = FMath::Clamp(Pawn->GetVelocity().Size2D() / 600.0f, 0.0f, 1.0f);
	const float Calm = ShotTime >= 0.0f ? 0.0f : 1.0f;
	const float Step = BobTime * 2.0f * PI * 1.6f;
	FVector2D Offset;
	Offset.X = FMath::Sin(Step) * WalkBobPixels * Run * Calm;
	Offset.Y = (FMath::Abs(FMath::Sin(Step)) * WalkBobPixels * 0.6f * Run
		+ FMath::Sin(BobTime * 2.0f * PI * 0.35f) * IdleSwayPixels * (1.0f - Run)) * Calm;
	// the arms are drawn running off the bottom and right edges of the frame: push every pose a little further
	// off-screen (at least as far as the bob can move it) so the bob never pulls those straight edges into view
	const FVector2D EdgeBleed(WalkBobPixels + EdgeBleedPixels, IdleSwayPixels + EdgeBleedPixels);
	SpriteWidget->SetPlacement(Offset + EdgeBleed, SpriteScreenScale);
}

void ADCShotgunWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bUseSpriteView)
	{
		return;
	}

	if (ShotTime >= 0.0f)
	{
		ShotTime += DeltaSeconds;

		// sequence over -> Idle (only once the peak recoil has been on screen)
		if (ShotTime >= GetShotAnimDuration() && bRecoilShown)
		{
			ShotTime = -1.0f;
		}
	}
	UpdateSpriteView(DeltaSeconds);
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

	// 2D view: this shot was accepted -> play the sequence once from the top; the flash shows on the input frame itself
	if (bUseSpriteView)
	{
		ShotTime = 0.0f;
		bRecoilShown = false;
		UpdateSpriteView(0.0f);
	}

	--CurrentBullets;

	if (CurrentBullets <= 0 && !IsOwnedByPlayer())
	{
		CurrentBullets = MagazineSize;
	}

	WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
}
