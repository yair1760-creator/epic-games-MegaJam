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

	// the 2D hands go away with the weapon (holstered, owner died); a holstered gun is not mid-shot or mid-reload
	if (bNewHidden)
	{
		ShotTime = -1.0f;
		ReloadAnimTime = -1.0f;
		CurrentPose = TEXT("Idle");
		if (SpriteWidget)
		{
			SpriteWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ADCShotgunWeapon::StartReload()
{
	const bool bWasReloading = bIsReloading;
	Super::StartReload();

	if (!bWasReloading && bIsReloading && bUseSpriteView)
	{
		ShotTime = -1.0f;
		bRecoilShown = false;
		ReloadAnimTime = 0.0f;
		UpdateSpriteView(0.0f);
	}
}

void ADCShotgunWeapon::FinishReload()
{
	ReloadAnimTime = -1.0f;
	Super::FinishReload();
	if (bUseSpriteView)
	{
		UpdateSpriteView(0.0f);
	}
}

void ADCShotgunWeapon::CancelReload()
{
	ReloadAnimTime = -1.0f;
	Super::CancelReload();
	if (bUseSpriteView)
	{
		UpdateSpriteView(0.0f);
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

	// the arms are drawn running off the bottom and right edges of the frame: push every pose a little further
	// off-screen (at least as far as the bob can move it) so the bob never pulls those straight edges into view
	const FVector2D EdgeBleed(WalkBobPixels + EdgeBleedPixels, IdleSwayPixels + EdgeBleedPixels);

	UTexture2D* Frame = SpriteIdle;
	FName Pose = TEXT("Idle");
	FVector2D AnimOffset = FVector2D::ZeroVector;
	float AnimScale = SpriteScreenScale;
	float AnimAngleDeg = 0.0f;

	if (ShotTime >= 0.0f)
	{
		// pose from the 12 fps tick grid: hard cuts, each drawing held for whole ticks
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
	else if (ReloadAnimTime >= 0.0f)
	{
		// Front-of-screen 2D reload sequence quantized to the 12 fps comic animation grid:
		// 1) Drop/tilt gun closer into view, 2) 3 shell-loading punches, 3) Rack pump up (SpriteRecoil), 4) Settle to Idle.
		const float FPS = FMath::Max(AnimFPS, 1.0f);
		const float Duration = FMath::Max(ReloadTime, 0.01f);
		const int32 TotalTicks = FMath::Max(1, FMath::RoundToInt(Duration * FPS));
		const int32 AnimTick = FMath::Clamp(FMath::FloorToInt(ReloadAnimTime * FPS), 0, TotalTicks - 1);
		const float T = TotalTicks > 1 ? float(AnimTick) / float(TotalTicks - 1) : 1.0f;

		UTexture2D* ReloadBaseFrame = SpriteReload ? SpriteReload.Get() : SpriteRecovery.Get();

		if (T < 0.20f)
		{
			const float Alpha = FMath::InterpEaseOut(0.0f, 1.0f, T / 0.20f, 2.0f);
			Pose = TEXT("ReloadDrop");
			Frame = ReloadBaseFrame;
			AnimScale = SpriteScreenScale * FMath::Lerp(1.0f, 1.12f, Alpha);
			AnimAngleDeg = FMath::Lerp(0.0f, -7.0f, Alpha);
			AnimOffset = FVector2D(FMath::Lerp(0.0f, 55.0f, Alpha), FMath::Lerp(0.0f, 95.0f, Alpha));
		}
		else if (T < 0.72f)
		{
			const float Sub = (T - 0.20f) / 0.52f;
			const float Cycle = FMath::Frac(Sub * 3.0f);
			const float Push = (Cycle < 0.45f) ? (Cycle / 0.45f) : (1.0f - (Cycle - 0.45f) / 0.55f);
			Pose = TEXT("ReloadLoad");
			Frame = (Push > 0.5f) ? SpriteIdle.Get() : ReloadBaseFrame;
			AnimScale = SpriteScreenScale * (1.12f + 0.05f * Push);
			AnimAngleDeg = -7.0f + 3.0f * Push;
			AnimOffset = FVector2D(55.0f - 15.0f * Push, 95.0f - 42.0f * Push);
		}
		else if (T < 0.89f)
		{
			const float Alpha = (T - 0.72f) / 0.17f;
			const float PumpArc = FMath::Sin(Alpha * PI);
			Pose = TEXT("ReloadPump");
			Frame = SpriteRecoil.Get();
			AnimScale = SpriteScreenScale * FMath::Lerp(1.12f, 1.14f, PumpArc);
			AnimAngleDeg = 0.0f;
			AnimOffset = FVector2D(10.0f, FMath::Lerp(55.0f, 12.0f, PumpArc));
		}
		else
		{
			const float Alpha = (T - 0.89f) / 0.11f;
			Pose = TEXT("ReloadSettle");
			Frame = (Alpha < 0.5f) ? ReloadBaseFrame : SpriteIdle.Get();
			AnimScale = SpriteScreenScale * FMath::Lerp(1.10f, 1.0f, Alpha);
			AnimAngleDeg = FMath::Lerp(-2.0f, 0.0f, Alpha);
			AnimOffset = FVector2D(FMath::Lerp(18.0f, 0.0f, Alpha), FMath::Lerp(28.0f, 0.0f, Alpha));
		}
	}

	CurrentPose = Pose;
	SpriteWidget->SetFrame(Frame ? Frame : SpriteIdle.Get());

	// walk bob / idle breathing while idle; shot and reload poses keep their animated placement
	BobTime += DeltaSeconds;
	const float Run = FMath::Clamp(Pawn->GetVelocity().Size2D() / 600.0f, 0.0f, 1.0f);
	const float Calm = (ShotTime >= 0.0f || ReloadAnimTime >= 0.0f) ? 0.0f : 1.0f;
	const float Step = BobTime * 2.0f * PI * 1.6f;
	FVector2D BobOffset;
	BobOffset.X = FMath::Sin(Step) * WalkBobPixels * Run * Calm;
	BobOffset.Y = (FMath::Abs(FMath::Sin(Step)) * WalkBobPixels * 0.6f * Run
		+ FMath::Sin(BobTime * 2.0f * PI * 0.35f) * IdleSwayPixels * (1.0f - Run)) * Calm;

	SpriteWidget->SetPlacement(BobOffset + AnimOffset + EdgeBleed, AnimScale, AnimAngleDeg);
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

	if (ReloadAnimTime >= 0.0f)
	{
		ReloadAnimTime += DeltaSeconds;
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

	UDCFXLibrary::PlayWorldSound(this, FireSound, MuzzleLoc, 0.55f, 0.05f, 0.02f, FireSoundPitch);
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
