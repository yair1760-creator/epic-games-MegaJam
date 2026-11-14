// Director's Cut - melee sword.

#include "DirectorsCut/Combat/DCSwordWeapon.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Combat/DCHitFeedbackComponent.h"
#include "DirectorsCut/Combat/DCWeaponSpriteWidget.h"
#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Rendering/DCSpriteFX.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "Engine/Texture2D.h"
#include "EpicGameJamGameCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ADCSwordWeapon::ADCSwordWeapon()
{
	DC_DEFAULT_SFX(SwingSound, "S_SwordSwing");
	DC_DEFAULT_SFX(HitSound, "S_SwordHit");
	HitSpriteFX = ADCFX_HitSplat::StaticClass();
	PrimaryActorTick.bCanEverTick = true;

	// a sword has no magazine: firing never needs ammo and reload does nothing
	MagazineSize = 0;
	ReloadTime = 0.0f;
	ProjectileClass = nullptr;

	HandPivot = CreateDefaultSubobject<USceneComponent>(TEXT("Hand Pivot"));
	HandPivot->SetupAttachment(GetRootComponent());

	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword Mesh"));
	SwordMesh->SetupAttachment(HandPivot);
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordMesh->SetCastShadow(false);
	SwordMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	SwordMesh->bOnlyOwnerSee = true;
	// the model's pivot is its centre; move it up so the grip (about 35 cm below centre) sits in the hand
	SwordMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 33.0f));

	SwordOutline = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword Outline"));
	SwordOutline->SetupAttachment(SwordMesh);
	SwordOutline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwordOutline->SetCastShadow(false);
	SwordOutline->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	SwordOutline->bOnlyOwnerSee = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SwordAsset(TEXT("/Game/DirectorsCut/Combat/Sword/SM_DC_Sword.SM_DC_Sword"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OutlineMat(TEXT("/Game/DirectorsCut/Combat/Sword/MI_DC_Sword_Outline.MI_DC_Sword_Outline"));
	if (SwordAsset.Succeeded())
	{
		SwordMesh->SetStaticMesh(SwordAsset.Object);
		SwordOutline->SetStaticMesh(SwordAsset.Object);
	}
	if (OutlineMat.Succeeded())
	{
		SwordOutline->SetMaterial(0, OutlineMat.Object);
	}

	// 2D first-person frames (cut from the designer's attack video)
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameIdle(TEXT("/Game/DirectorsCut/Combat/Sword/Sprites/T_Sword_Idle.T_Sword_Idle"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameWindup(TEXT("/Game/DirectorsCut/Combat/Sword/Sprites/T_Sword_Windup.T_Sword_Windup"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameSwing(TEXT("/Game/DirectorsCut/Combat/Sword/Sprites/T_Sword_Swing.T_Sword_Swing"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameFollow(TEXT("/Game/DirectorsCut/Combat/Sword/Sprites/T_Sword_Follow.T_Sword_Follow"));
	if (FrameIdle.Succeeded())   SpriteIdle = FrameIdle.Object;
	if (FrameWindup.Succeeded()) SpriteWindup = FrameWindup.Object;
	if (FrameSwing.Succeeded())  SpriteSwing = FrameSwing.Object;
	if (FrameFollow.Succeeded()) SpriteFollow = FrameFollow.Object;
}

float ADCSwordWeapon::GetHitDelay() const
{
	// 2D view: the blade connects when the swing smear appears (end of the windup ticks)
	return bUseSpriteView ? float(WindupTicks) / FMath::Max(AnimFPS, 1.0f) : HitDelay;
}

void ADCSwordWeapon::RemoveSpriteWidget()
{
	if (SpriteWidget)
	{
		SpriteWidget->RemoveFromParent();
		SpriteWidget = nullptr;
	}
}

void ADCSwordWeapon::UpdateSpriteView(float DeltaSeconds)
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

	// pose from the 12 fps tick grid: hard cuts, each pose held for whole ticks
	UTexture2D* Frame = SpriteIdle;
	FName Pose = TEXT("Idle");
	if (SwingTime >= 0.0f)
	{
		const int32 SmearStart = WindupTicks;
		const int32 FollowStart = WindupTicks + SmearTicks;
		const int32 End = FollowStart + FollowTicks;
		int32 Tick = FMath::FloorToInt(SwingTime * FMath::Max(AnimFPS, 1.0f));

		// a slow frame may jump straight past the smear: show it anyway, it is the attack
		if (Tick >= FollowStart && !bSmearShown)
		{
			Tick = SmearStart;
		}

		if (Tick < SmearStart)        { Frame = SpriteWindup; Pose = TEXT("Windup"); }
		else if (Tick < FollowStart)  { Frame = SpriteSwing;  Pose = TEXT("Swing"); bSmearShown = true; }
		else if (Tick < End)          { Frame = SpriteFollow; Pose = TEXT("Follow"); }
	}
	CurrentPose = Pose;
	SpriteWidget->SetFrame(Frame ? Frame : SpriteIdle.Get());

	// walk bob / idle breathing while idle; the attack poses keep the exact drawn placement
	BobTime += DeltaSeconds;
	const float Run = FMath::Clamp(Pawn->GetVelocity().Size2D() / 600.0f, 0.0f, 1.0f);
	const float Calm = SwingTime >= 0.0f ? 0.0f : 1.0f;
	const float Step = BobTime * 2.0f * PI * 1.6f;
	FVector2D Offset;
	Offset.X = FMath::Sin(Step) * WalkBobPixels * Run * Calm;
	Offset.Y = (FMath::Abs(FMath::Sin(Step)) * WalkBobPixels * 0.6f * Run
		+ FMath::Sin(BobTime * 2.0f * PI * 0.35f) * IdleSwayPixels * (1.0f - Run)) * Calm;
	// the arm is drawn running off the frame's bottom-right corner: push every pose a little further off-screen
	// (at least as far as the bob can move it) so the bob never pulls the frame's straight edge into view
	const FVector2D EdgeBleed(WalkBobPixels + EdgeBleedPixels, IdleSwayPixels + EdgeBleedPixels);
	SpriteWidget->SetPlacement(Offset + EdgeBleed, SpriteScreenScale);
}

void ADCSwordWeapon::BeginPlay()
{
	// the base class reads these when firing; in the 2D view one attack plays fully before the next is accepted
	RefireRate = bUseSpriteView ? FMath::Max(Cooldown, GetAttackDuration()) : Cooldown;
	bFullAuto = bHoldToSwing;

	Super::BeginPlay();   // attaches the (empty) skeletal meshes to the hand socket

	AttachToOwnerCamera();

	// 2D view replaces the 3D model in first person
	SwordMesh->SetVisibility(!bUseSpriteView, true);
}

void ADCSwordWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(HitTimer);
	RemoveSpriteWidget();
	SetOwnerArmsHidden(false);
	Super::EndPlay(EndPlayReason);
}

void ADCSwordWeapon::AttachToOwnerCamera()
{
	const AEpicGameJamGameCharacter* Char = Cast<AEpicGameJamGameCharacter>(GetOwner());
	if (Char && Char->GetFirstPersonCameraComponent())
	{
		HandPivot->AttachToComponent(Char->GetFirstPersonCameraComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		HandPivot->SetRelativeLocation(HoldOffset);
		HandPivot->SetRelativeRotation(RestRotation);
	}
}

void ADCSwordWeapon::SetActorHiddenInGame(bool bNewHidden)
{
	Super::SetActorHiddenInGame(bNewHidden);

	// the template arms are posed for a rifle: hide them while the sword is out
	SetOwnerArmsHidden(!bNewHidden);

	// the 2D hand goes away with the weapon (holstered, owner died)
	if (bNewHidden && SpriteWidget)
	{
		SpriteWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ADCSwordWeapon::SetOwnerArmsHidden(bool bArmsHidden)
{
	if (AEpicGameJamGameCharacter* Char = Cast<AEpicGameJamGameCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Arms = Char->GetFirstPersonMesh())
		{
			Arms->SetHiddenInGame(bArmsHidden, false);
		}
	}
}

void ADCSwordWeapon::Fire()
{
	if (!bIsFiring || !PawnOwner)
	{
		return;
	}

	TimeOfLastShot = GetWorld()->GetTimeSeconds();
	SwingTime = 0.0f;
	bSmearShown = false;
	if (bUseSpriteView)
	{
		UpdateSpriteView(0.0f);   // Windup shows on the input frame itself
	}
	OnSwing.Broadcast();
	UDCFXLibrary::PlayUISound(this, SwingSound, 0.55f);
	if (SwingFX)
	{
		FVector EyeLoc;
		FRotator EyeRot;
		PawnOwner->GetActorEyesViewPoint(EyeLoc, EyeRot);
		UDCFXLibrary::SpawnWorldFX(this, SwingFX, EyeLoc + EyeRot.Vector() * 80.0f, EyeRot);
	}

	// AI can hear swings too
	MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, NoiseOwnerTag);

	if (GetHitDelay() > 0.0f)
	{
		GetWorldTimerManager().SetTimer(HitTimer, this, &ADCSwordWeapon::DoHitSweep, GetHitDelay(), false);
	}
	else
	{
		DoHitSweep();
	}

	// keep swinging while held, or tell the owner the cooldown ended
	if (bFullAuto)
	{
		GetWorldTimerManager().SetTimer(RefireTimer, this, &ADCSwordWeapon::Fire, RefireRate, false);
	}
	else
	{
		GetWorldTimerManager().SetTimer(RefireTimer, this, &ADCSwordWeapon::FireCooldownExpired, RefireRate, false);
	}
}

void ADCSwordWeapon::DoHitSweep()
{
	APawn* Pawn = PawnOwner.Get();
	if (!Pawn)
	{
		return;
	}

	// sweep from the eyes along the view direction
	FVector ViewLoc;
	FRotator ViewRot;
	Pawn->GetActorEyesViewPoint(ViewLoc, ViewRot);
	if (const AEpicGameJamGameCharacter* Char = Cast<AEpicGameJamGameCharacter>(Pawn))
	{
		if (const UCameraComponent* Cam = Char->GetFirstPersonCameraComponent())
		{
			ViewLoc = Cam->GetComponentLocation();
			ViewRot = Cam->GetComponentRotation();
		}
	}
	const FVector Start = ViewLoc + ViewRot.Vector() * SwingRadius * 0.5f;
	const FVector End = ViewLoc + ViewRot.Vector() * FMath::Max(Range - SwingRadius, SwingRadius);

	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCSwordSwing), false, Pawn);
	Params.AddIgnoredActor(this);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Objects, FCollisionShape::MakeSphere(SwingRadius), Params);

	if (bDebugDraw)
	{
		DrawDebugCapsule(GetWorld(), (Start + End) * 0.5f, (End - Start).Size() * 0.5f + SwingRadius, SwingRadius,
			FRotationMatrix::MakeFromZ(End - Start).ToQuat(), Hits.Num() ? FColor::Red : FColor::Green, false, 1.0f);
	}

	TSet<AActor*> Done;
	int32 Count = 0;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || Done.Contains(Target) || Target == Pawn || !Target->CanBeDamaged())
		{
			continue;
		}

		// only things that can take damage: DC Health owners and pawns (the Shooter characters)
		if (!UDCHealthComponent::FindHealth(Target) && !Target->IsA<APawn>())
		{
			continue;
		}

		Done.Add(Target);

		UGameplayStatics::ApplyDamage(Target, Damage, Pawn->GetController(), Pawn, DamageTypeClass);
		OnHit.Broadcast(Target, Damage);
		{
			const FVector ImpactAt = Hit.bBlockingHit || !Hit.ImpactPoint.IsNearlyZero() ? FVector(Hit.ImpactPoint) : Target->GetActorLocation();
			UDCFXLibrary::PlayWorldSound(this, HitSound, ImpactAt, 0.50f);
			UDCFXLibrary::SpawnWorldFX(this, HitFX, ImpactAt, (ViewLoc - ImpactAt).Rotation());
			UDCFXLibrary::SpawnSpriteFX(this, HitSpriteFX, ImpactAt, 1.0f, 25.0f);
		}

		if (Knockback > 0.0f)
		{
			if (ACharacter* HitChar = Cast<ACharacter>(Target))
			{
				FVector Push = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D() * Knockback;
				Push.Z = Knockback * 0.35f;
				HitChar->LaunchCharacter(Push, true, true);
			}
		}

		if (++Count >= MaxTargetsPerSwing)
		{
			break;
		}
	}

	// a little kick for the player when the blade connects
	if (Count > 0 && HitShakeDegrees > 0.0f)
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
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
					Shake->Trigger(HitShakeDegrees, 0.15f);
				}
			}
		}
	}

	UE_LOG(LogDCCombat, Verbose, TEXT("Sword swing hit %d target(s)"), Count);
}

void ADCSwordWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (SwingTime >= 0.0f)
	{
		SwingTime += DeltaSeconds;
	}

	// 2D view: frames + bob on screen
	if (bUseSpriteView)
	{
		// attack over -> Idle (only once the smear has been on screen)
		if (SwingTime >= GetAttackDuration() && bSmearShown)
		{
			SwingTime = -1.0f;
		}
		UpdateSpriteView(DeltaSeconds);
		return;
	}

	if (SwingTime < 0.0f)
	{
		return;
	}

	const float T = FMath::Clamp(SwingTime / SwingDuration, 0.0f, 1.0f);

	// fast slash out (first 40%), slower return
	float Alpha;
	if (T < 0.4f)
	{
		Alpha = FMath::InterpEaseOut(0.0f, 1.0f, T / 0.4f, 2.0f);
	}
	else
	{
		Alpha = FMath::InterpEaseInOut(1.0f, 0.0f, (T - 0.4f) / 0.6f, 2.0f);
	}

	const FQuat Rest = RestRotation.Quaternion();
	const FQuat End = SwingEndRotation.Quaternion();
	HandPivot->SetRelativeRotation(FQuat::Slerp(Rest, End, Alpha));
	HandPivot->SetRelativeLocation(HoldOffset + FVector(8.0f, -12.0f, -4.0f) * Alpha);

	if (T >= 1.0f)
	{
		SwingTime = -1.0f;
		HandPivot->SetRelativeRotation(RestRotation);
		HandPivot->SetRelativeLocation(HoldOffset);
	}
}
