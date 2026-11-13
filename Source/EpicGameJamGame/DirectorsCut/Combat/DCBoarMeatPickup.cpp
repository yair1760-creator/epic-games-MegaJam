// Director's Cut - collectible boar meat / ham dropped during Beat 2.

#include "DirectorsCut/Combat/DCBoarMeatPickup.h"
#include "Quest/DCQuestSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADCBoarMeatPickup::ADCBoarMeatPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(PickupRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// a camera-facing ham picture, sitting on the ground and bobbing with the actor
	Sprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Sprite"));
	Sprite->SetupAttachment(CollisionSphere);
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -30.0f));
	Sprite->FrontSize = FVector2D(70.0f, 70.0f);
	Sprite->bPivotAtBottom = true;
	static ConstructorHelpers::FObjectFinder<UTexture2D> MeatArt(TEXT("/Game/DirectorsCut/Characters/Boar/Sprites/T_DC_BoarMeat_Placeholder.T_DC_BoarMeat_Placeholder"));
	if (MeatArt.Succeeded())
	{
		Sprite->FrontTexture = MeatArt.Object;
	}

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow Light"));
	GlowLight->SetupAttachment(CollisionSphere);
	GlowLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.35f));
	GlowLight->SetIntensity(2200.0f);
	GlowLight->SetAttenuationRadius(350.0f);
}

void ADCBoarMeatPickup::BeginPlay()
{
	Super::BeginPlay();
	BaseZ = GetActorLocation().Z;
	CollisionSphere->SetSphereRadius(PickupRadius, true);
	CollisionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADCBoarMeatPickup::HandleOverlap);
}

void ADCBoarMeatPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;

	FVector Loc = GetActorLocation();
	Loc.Z = BaseZ + FMath::Sin(Age * 4.0f) * 12.0f;

	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const float Dist = FVector::Dist2D(Loc, Player->GetActorLocation());
		if (Age > 0.35f && Dist <= PickupRadius)
		{
			SetActorLocation(Loc);
			Collect();
			return;
		}
		if (Age > 0.35f && Dist <= MagnetRadius)
		{
			const FVector Dir = (Player->GetActorLocation() - Loc).GetSafeNormal2D();
			Loc += Dir * MagnetSpeed * DeltaSeconds;
		}
	}

	SetActorLocation(Loc);
}

void ADCBoarMeatPickup::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bCollected || Age < 0.25f)
	{
		return;
	}
	if (APawn* Pawn = Cast<APawn>(OtherActor); Pawn && Pawn->IsPlayerControlled())
	{
		Collect();
	}
}

void ADCBoarMeatPickup::Collect()
{
	if (bCollected)
	{
		return;
	}
	bCollected = true;

	if (UDCQuestSubsystem* Quest = GetWorld() ? GetWorld()->GetSubsystem<UDCQuestSubsystem>() : nullptr)
	{
		Quest->ReportEvent(EventTag);
	}
	Destroy();
}
