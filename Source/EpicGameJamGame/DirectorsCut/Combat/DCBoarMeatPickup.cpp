// Director's Cut - collectible boar meat / ham dropped during Beat 2.

#include "DirectorsCut/Combat/DCBoarMeatPickup.h"
#include "Quest/DCQuestSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MeatMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Landmark.MI_LD_Landmark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BoneMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_White.MI_LD_White"));

	MeatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Meat Mesh"));
	MeatMesh->SetupAttachment(CollisionSphere);
	MeatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeatMesh->SetRelativeScale3D(FVector(0.55f, 0.36f, 0.28f));
	if (Cube.Succeeded())
	{
		MeatMesh->SetStaticMesh(Cube.Object);
	}
	if (MeatMat.Succeeded())
	{
		MeatMesh->SetMaterial(0, MeatMat.Object);
	}

	BoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bone Mesh"));
	BoneMesh->SetupAttachment(MeatMesh);
	BoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoneMesh->SetRelativeLocation(FVector(55.0f, 0.0f, 0.0f));
	BoneMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	BoneMesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.65f));
	if (Cyl.Succeeded())
	{
		BoneMesh->SetStaticMesh(Cyl.Object);
	}
	if (BoneMat.Succeeded())
	{
		BoneMesh->SetMaterial(0, BoneMat.Object);
	}

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow Light"));
	GlowLight->SetupAttachment(CollisionSphere);
	GlowLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.35f));
	GlowLight->SetIntensity(2200.0f);
	GlowLight->SetAttenuationRadius(350.0f);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label Text"));
	LabelText->SetupAttachment(CollisionSphere);
	LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetWorldSize(24.0f);
	LabelText->SetText(FText::FromString(TEXT("BOAR HAM")));
	LabelText->SetTextRenderColor(FColor::Yellow);
	LabelText->SetUsingAbsoluteRotation(true);
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

	AddActorLocalRotation(FRotator(0.0f, 110.0f * DeltaSeconds, 0.0f));

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

		if (LabelText)
		{
			LabelText->SetWorldRotation(FRotator(0.0f, (Player->GetActorLocation() - LabelText->GetComponentLocation()).Rotation().Yaw, 0.0f));
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
