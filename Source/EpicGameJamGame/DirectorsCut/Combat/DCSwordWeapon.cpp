// Director's Cut - melee sword.

#include "DirectorsCut/Combat/DCSwordWeapon.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Combat/DCHitFeedbackComponent.h"
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
}

void ADCSwordWeapon::BeginPlay()
{
	// the base class reads these when firing
	RefireRate = Cooldown;
	bFullAuto = bHoldToSwing;

	Super::BeginPlay();   // attaches the (empty) skeletal meshes to the hand socket

	AttachToOwnerCamera();
}

void ADCSwordWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(HitTimer);
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
	OnSwing.Broadcast();

	// AI can hear swings too
	MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, NoiseOwnerTag);

	if (HitDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(HitTimer, this, &ADCSwordWeapon::DoHitSweep, HitDelay, false);
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

	if (SwingTime < 0.0f)
	{
		return;
	}

	SwingTime += DeltaSeconds;
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
