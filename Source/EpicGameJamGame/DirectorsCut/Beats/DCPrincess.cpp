// Director's Cut - Beat 4 Princess actor guarded by the King Boar.

#include "DirectorsCut/Beats/DCPrincess.h"
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

ADCPrincess::ADCPrincess()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	RescueTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("Rescue Trigger"));
	RescueTrigger->SetupAttachment(Root);
	RescueTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	RescueTrigger->InitSphereRadius(280.0f);
	RescueTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RescueTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RescueTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Chamfer(TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PinkMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_PG_Pink.MI_LD_PG_Pink"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Breadcrumb.MI_LD_Breadcrumb"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WhiteMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_White.MI_LD_White"));

	PrincessDress = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Princess Dress"));
	PrincessDress->SetupAttachment(Root);
	PrincessDress->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	PrincessDress->SetRelativeScale3D(FVector(0.85f, 0.85f, 1.45f));
	PrincessDress->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Cyl.Succeeded()) PrincessDress->SetStaticMesh(Cyl.Object);
	if (PinkMat.Succeeded()) PrincessDress->SetMaterial(0, PinkMat.Object);

	PrincessHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Princess Head"));
	PrincessHead->SetupAttachment(PrincessDress);
	PrincessHead->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	PrincessHead->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.4f));
	PrincessHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Chamfer.Succeeded()) PrincessHead->SetStaticMesh(Chamfer.Object);
	if (WhiteMat.Succeeded()) PrincessHead->SetMaterial(0, WhiteMat.Object);

	PrincessCrown = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Princess Crown"));
	PrincessCrown->SetupAttachment(PrincessHead);
	PrincessCrown->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	PrincessCrown->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.45f));
	PrincessCrown->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Cyl.Succeeded()) PrincessCrown->SetStaticMesh(Cyl.Object);
	if (GoldMat.Succeeded()) PrincessCrown->SetMaterial(0, GoldMat.Object);

	RomanceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Romance Light"));
	RomanceLight->SetupAttachment(Root);
	RomanceLight->SetRelativeLocation(FVector(0.0f, 0.0f, 240.0f));
	RomanceLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.8f));
	RomanceLight->SetIntensity(4500.0f);
	RomanceLight->SetAttenuationRadius(750.0f);

	PrincessLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Princess Label"));
	PrincessLabel->SetupAttachment(Root);
	PrincessLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 340.0f));
	PrincessLabel->SetHorizontalAlignment(EHTA_Center);
	PrincessLabel->SetWorldSize(42.0f);
	PrincessLabel->SetText(FText::FromString(TEXT("PRINCESS (GUARDED)")));
	PrincessLabel->SetTextRenderColor(FColor(255, 130, 210));
	PrincessLabel->SetUsingAbsoluteRotation(true);

	Tags.Add(TEXT("Princess"));
}

void ADCPrincess::BeginPlay()
{
	Super::BeginPlay();
	RescueTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADCPrincess::HandleOverlap);
}

void ADCPrincess::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BobTime += DeltaSeconds;

	if (PrincessDress && !bRescued)
	{
		PrincessDress->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f + FMath::Sin(BobTime * 3.0f) * 8.0f));
	}
	if (PrincessLabel)
	{
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			PrincessLabel->SetWorldRotation(FRotator(0.0f, (Player->GetActorLocation() - PrincessLabel->GetComponentLocation()).Rotation().Yaw, 0.0f));
		}
	}
}

void ADCPrincess::UnlockPrincess()
{
	bLocked = false;
	if (PrincessLabel)
	{
		PrincessLabel->SetText(FText::FromString(TEXT("RESCUE THE PRINCESS!")));
		PrincessLabel->SetTextRenderColor(FColor(120, 255, 160));
	}

	// If player is already standing inside the rescue trigger when unlocking, complete immediately
	TArray<AActor*> Overlapping;
	RescueTrigger->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* A : Overlapping)
	{
		if (APawn* P = Cast<APawn>(A); P && P->IsPlayerControlled())
		{
			HandleOverlap(RescueTrigger, P, nullptr, 0, false, FHitResult());
			break;
		}
	}
}

void ADCPrincess::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bRescued)
	{
		return;
	}
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (bLocked)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastLockedPromptTime >= 6.0f)
		{
			LastLockedPromptTime = Now;
			OnLockedAttempt.Broadcast();
		}
		return;
	}

	bRescued = true;
	if (PrincessLabel)
	{
		PrincessLabel->SetText(FText::FromString(TEXT("PRINCESS RESCUED!")));
	}
	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReportEvent(FName("Rescue.Princess"));
	}
	OnPrincessRescued.Broadcast();
}
