// Director's Cut - Beat 4 lockable double doors on the Princess shed.

#include "DirectorsCut/Beats/DCShedDoors.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADCShedDoors::ADCShedDoors()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Chamfer(TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LilacMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_PG_Lilac.MI_LD_PG_Lilac"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Breadcrumb.MI_LD_Breadcrumb"));

	// Hinges sit at the inner edges of the 260 cm doorway (y = +-125 cm)
	LeftHinge = CreateDefaultSubobject<USceneComponent>(TEXT("Left Hinge"));
	LeftHinge->SetupAttachment(Root);
	LeftHinge->SetRelativeLocation(FVector(0.0f, -125.0f, 0.0f));

	RightHinge = CreateDefaultSubobject<USceneComponent>(TEXT("Right Hinge"));
	RightHinge->SetupAttachment(Root);
	RightHinge->SetRelativeLocation(FVector(0.0f, 125.0f, 0.0f));

	LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Left Door"));
	LeftDoorMesh->SetupAttachment(LeftHinge);
	LeftDoorMesh->SetRelativeLocation(FVector(0.0f, 62.0f, 155.0f));
	LeftDoorMesh->SetRelativeScale3D(FVector(0.18f, 1.24f, 3.05f));
	if (Chamfer.Succeeded()) LeftDoorMesh->SetStaticMesh(Chamfer.Object);
	if (LilacMat.Succeeded()) LeftDoorMesh->SetMaterial(0, LilacMat.Object);

	RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Right Door"));
	RightDoorMesh->SetupAttachment(RightHinge);
	RightDoorMesh->SetRelativeLocation(FVector(0.0f, -62.0f, 155.0f));
	RightDoorMesh->SetRelativeScale3D(FVector(0.18f, 1.24f, 3.05f));
	if (Chamfer.Succeeded()) RightDoorMesh->SetStaticMesh(Chamfer.Object);
	if (LilacMat.Succeeded()) RightDoorMesh->SetMaterial(0, LilacMat.Object);

	// Padlock diamond badge on the front of the closed doors
	LockBadgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lock Badge"));
	LockBadgeMesh->SetupAttachment(Root);
	LockBadgeMesh->SetRelativeLocation(FVector(16.0f, 0.0f, 155.0f));
	LockBadgeMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 45.0f));
	LockBadgeMesh->SetRelativeScale3D(FVector(0.18f, 0.45f, 0.45f));
	LockBadgeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Chamfer.Succeeded()) LockBadgeMesh->SetStaticMesh(Chamfer.Object);
	if (GoldMat.Succeeded()) LockBadgeMesh->SetMaterial(0, GoldMat.Object);

	LockGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lock Glow"));
	LockGlow->SetupAttachment(Root);
	LockGlow->SetRelativeLocation(FVector(70.0f, 0.0f, 170.0f));
	LockGlow->SetLightColor(FLinearColor(1.0f, 0.78f, 0.25f));
	LockGlow->SetIntensity(2200.0f);
	LockGlow->SetAttenuationRadius(420.0f);

	DoorBlocker = CreateDefaultSubobject<UBoxComponent>(TEXT("Door Blocker"));
	DoorBlocker->SetupAttachment(Root);
	DoorBlocker->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
	DoorBlocker->SetBoxExtent(FVector(28.0f, 132.0f, 155.0f));
	DoorBlocker->SetCollisionProfileName(TEXT("BlockAll"));

	LockPromptTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("Lock Prompt Trigger"));
	LockPromptTrigger->SetupAttachment(Root);
	LockPromptTrigger->SetRelativeLocation(FVector(85.0f, 0.0f, 120.0f));
	LockPromptTrigger->InitSphereRadius(185.0f);
	LockPromptTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockPromptTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	LockPromptTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Tags.Add(TEXT("ShedDoors"));
}

void ADCShedDoors::BeginPlay()
{
	Super::BeginPlay();
	LockPromptTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADCShedDoors::HandlePromptOverlap);
	ResetLocked();
}

void ADCShedDoors::ResetLocked()
{
	bLocked = true;
	bOpening = false;
	CurrentAngle = 0.0f;

	if (LeftHinge) LeftHinge->SetRelativeRotation(FRotator::ZeroRotator);
	if (RightHinge) RightHinge->SetRelativeRotation(FRotator::ZeroRotator);
	if (LeftDoorMesh) LeftDoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (RightDoorMesh) RightDoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (DoorBlocker) DoorBlocker->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (LockPromptTrigger) LockPromptTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	if (LockBadgeMesh) LockBadgeMesh->SetVisibility(true);
	if (LockGlow) LockGlow->SetVisibility(true);
}

void ADCShedDoors::UnlockAndOpen()
{
	if (!bLocked && bOpening)
	{
		return;
	}
	bLocked = false;
	bOpening = true;

	if (DoorBlocker) DoorBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (LeftDoorMesh) LeftDoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (RightDoorMesh) RightDoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (LockPromptTrigger) LockPromptTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (LockBadgeMesh) LockBadgeMesh->SetVisibility(false);
	if (LockGlow) LockGlow->SetVisibility(false);
}

void ADCShedDoors::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bOpening && !FMath::IsNearlyEqual(CurrentAngle, OpenAngle, 0.25f))
	{
		CurrentAngle = FMath::FInterpTo(CurrentAngle, OpenAngle, DeltaSeconds, OpenSpeed);
		if (LeftHinge) LeftHinge->SetRelativeRotation(FRotator(0.0f, -CurrentAngle, 0.0f));
		if (RightHinge) RightHinge->SetRelativeRotation(FRotator(0.0f, CurrentAngle, 0.0f));
	}
}

void ADCShedDoors::HandlePromptOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bLocked)
	{
		return;
	}
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastPromptTime >= 6.0f)
	{
		LastPromptTime = Now;
		OnLockedAttempt.Broadcast();
	}
}
