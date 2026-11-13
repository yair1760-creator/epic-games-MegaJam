// Director's Cut - boar meat pickup.

#include "DirectorsCut/AI/DCBoarMeat.h"
#include "DirectorsCut/AI/DCBoarSubsystem.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Quest/DCQuestSubsystem.h"
#include "ShooterCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ADCBoarMeat::ADCBoarMeat()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->InitSphereRadius(70.0f);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);

	Sprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Sprite"));
	Sprite->SetupAttachment(Trigger);
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
	Sprite->FrontSize = FVector2D(55.0f, 55.0f);
	Sprite->bPivotAtBottom = true;

	static ConstructorHelpers::FObjectFinder<UTexture2D> MeatArt(TEXT("/Game/DirectorsCut/Characters/Boar/Sprites/T_DC_BoarMeat_Placeholder.T_DC_BoarMeat_Placeholder"));
	if (MeatArt.Succeeded())
	{
		Sprite->FrontTexture = MeatArt.Object;
	}

	Tags.Add(TEXT("BoarMeat"));
}

void ADCBoarMeat::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ADCBoarMeat::OnTriggerOverlap);

	GroundZ = FindGroundZ(GetActorLocation());

	if (Velocity.IsNearlyZero())
	{
		Pop(FVector::ZeroVector);
	}
	if (Lifetime > 0.0f)
	{
		SetLifeSpan(Lifetime);
	}
}

float ADCBoarMeat::FindGroundZ(const FVector& At) const
{
	// level geometry only: never the dead boar, the player or other pickups
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DCMeatGround), false, this);
	FHitResult Hit;
	const FVector Start = At + FVector(0.0f, 0.0f, 30.0f);
	return GetWorld()->LineTraceSingleByObjectType(Hit, Start, Start - FVector(0.0f, 0.0f, 3000.0f), Objects, Params)
		? Hit.ImpactPoint.Z : At.Z - 40.0f;
}

void ADCBoarMeat::Pop(const FVector& AwayFrom)
{
	FVector Out = (GetActorLocation() - AwayFrom).GetSafeNormal2D();
	if (AwayFrom.IsZero() || Out.IsNearlyZero())
	{
		Out = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
	}
	Velocity = Out * PopOutSpeed + FVector(0.0f, 0.0f, PopUpSpeed);
	bLanded = false;
}

void ADCBoarMeat::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;

	const float Lift = Trigger->GetUnscaledSphereRadius() * 0.5f;

	if (!bLanded)
	{
		// simple ballistic pop, stops on the ground under where it is now (no physics body needed)
		Velocity.Z -= 980.0f * DeltaSeconds;
		FVector L = GetActorLocation() + Velocity * DeltaSeconds;
		GroundZ = FindGroundZ(L);
		if (L.Z <= GroundZ + Lift && Velocity.Z < 0.0f)
		{
			L.Z = GroundZ + Lift;
			bLanded = true;
		}
		SetActorLocation(L, /*bSweep*/ false);
		return;
	}
	// landed: gentle bob so it reads as a pickup
	{
		FVector L = GetActorLocation();
		L.Z = GroundZ + Lift + BobHeight * (0.5f + 0.5f * FMath::Sin(Age * BobSpeed * 2.0f * PI));
		SetActorLocation(L, false);
	}

	// the player may already be standing on it when the pickup delay ends
	if (!bCollected && Age >= PickupDelay)
	{
		TArray<AActor*> Overlapping;
		Trigger->GetOverlappingActors(Overlapping, APawn::StaticClass());
		for (AActor* A : Overlapping)
		{
			TryCollect(A);
			if (bCollected)
			{
				break;
			}
		}
	}
}

void ADCBoarMeat::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryCollect(OtherActor);
}

void ADCBoarMeat::TryCollect(AActor* Other)
{
	if (bCollected || Age < PickupDelay || !Other)
	{
		return;
	}

	// only the player picks meat up (the Shooter player character, or anything tagged Player)
	APawn* Pawn = Cast<APawn>(Other);
	const AShooterCharacter* Shooter = Cast<AShooterCharacter>(Other);
	const bool bIsPlayer = (Shooter && Shooter->IsPlayerControlled() && !Shooter->IsDead()) || Other->ActorHasTag(TEXT("Player"));
	if (!bIsPlayer)
	{
		return;
	}

	bCollected = true;   // guard: one report per piece, even with several overlaps in one frame

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReportEvent(QuestEventTag);
	}
	if (UDCBoarSubsystem* Boars = GetWorld()->GetSubsystem<UDCBoarSubsystem>())
	{
		Boars->NotifyMeatCollected(this, Pawn);
	}
	Destroy();
}
