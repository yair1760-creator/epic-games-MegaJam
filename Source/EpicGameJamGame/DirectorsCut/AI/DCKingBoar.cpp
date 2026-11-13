// Director's Cut - Beat 4 King Boar guarding the Princess shed.

#include "DirectorsCut/AI/DCKingBoar.h"
#include "DirectorsCut/AI/DCBoarAIController.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Combat/DCFloatingHPBar.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADCKingBoar::ADCKingBoar()
{
	SetActorScale3D(FVector(2.5f));

	Health->MaxHP = 400.0f;
	FleeHPFraction = 0.0f;
	ChargeSpeed = 580.0f;
	BiteDamage = 25.0f;
	BiteRange = 110.0f;
	LoseInterestDistance = 6000.0f;
	MaxChaseTime = 60.0f;
	WanderRadius = 700.0f;
	KillEventTag = FName("Kill.KingBoar");
	CorpseTime = 5.0f;

	CrownMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crown Mesh"));
	CrownMesh->SetupAttachment(GetRootComponent());
	CrownMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrownMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
	CrownMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.4f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Breadcrumb.MI_LD_Breadcrumb"));
	if (Cyl.Succeeded())
	{
		CrownMesh->SetStaticMesh(Cyl.Object);
	}
	if (GoldMat.Succeeded())
	{
		CrownMesh->SetMaterial(0, GoldMat.Object);
	}

	if (StateText)
	{
		StateText->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
		StateText->SetWorldSize(34.0f);
		StateText->SetText(FText::FromString(TEXT("KING BOAR")));
		StateText->SetTextRenderColor(FColor::Yellow);
	}

	Tags.Add(TEXT("KingBoar"));
}

void ADCKingBoar::BeginPlay()
{
	Super::BeginPlay();

	if (StateText)
	{
		StateText->SetVisibility(true);
		StateText->SetText(FText::FromString(TEXT("KING BOAR")));
		StateText->SetTextRenderColor(FColor::Yellow);
	}
}

void ADCKingBoar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CrownMesh)
	{
		CrownMesh->AddLocalRotation(FRotator(0.0f, 60.0f * DeltaSeconds, 0.0f));
	}

	if (StateText && !IsDead())
	{
		StateText->SetText(FText::FromString(TEXT("KING BOAR")));
	}

	// Aggro automatically if player approaches within guard range
	if (!IsDead() && (GetBoarState() == EDCBoarState::Idle || GetBoarState() == EDCBoarState::Wander))
	{
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) <= 1800.0f)
			{
				if (ADCBoarAIController* AI = Cast<ADCBoarAIController>(GetController()))
				{
					AI->BecomeAngry(Player);
				}
			}
		}
	}
}
