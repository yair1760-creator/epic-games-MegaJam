// Director's Cut - Beat 4 King Boar guarding the Princess shed.

#include "DirectorsCut/AI/DCKingBoar.h"
#include "DirectorsCut/AI/DCBoarAIController.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADCKingBoar::ADCKingBoar()
{
	SetActorScale3D(FVector(2.5f));

	Health->MaxHP = 900.0f;   // rifle (25 x 10/s) needs ~36 hits, about 1 magazine + a reload
	FleeHPFraction = 0.0f;
	ChargeSpeed = 580.0f;
	BiteDamage = 25.0f;
	BiteRange = 110.0f;
	LoseInterestDistance = 6000.0f;
	MaxChaseTime = 60.0f;
	WanderRadius = 700.0f;
	KillEventTag = FName("Kill.KingBoar");
	CorpseTime = 5.0f;
	SoundVolume = 1.6f;   // same boar sounds, bigger and deeper
	SoundPitch = 0.7f;

	// gold boar
	if (Sprite)
	{
		Sprite->Tint = KingTint;
		Sprite->Brightness = KingBrightness;
	}

	// crown: its own camera-facing card on the boar card, nudged toward the camera so it always draws in front of the head
	CrownSprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Crown Sprite"));
	CrownSprite->SetupAttachment(Sprite ? static_cast<USceneComponent*>(Sprite) : GetRootComponent());
	CrownSprite->SetRelativeLocation(CrownOffset);
	CrownSprite->FrontSize = FVector2D(52.0f, 39.0f);
	CrownSprite->bPivotAtBottom = true;
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrownArt(TEXT("/Game/DirectorsCut/Characters/Boar/Sprites/T_DC_KingCrown_Placeholder.T_DC_KingCrown_Placeholder"));
	if (CrownArt.Succeeded())
	{
		CrownSprite->FrontTexture = CrownArt.Object;
	}

	if (StateText)
	{
		StateText->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));   // debug label (off unless bShowDebugState)
	}

	Tags.Add(TEXT("KingBoar"));
}

void ADCKingBoar::BeginPlay()
{
	Super::BeginPlay();

	if (Sprite)
	{
		Sprite->SetTint(KingTint, KingBrightness);
	}
	if (CrownSprite)
	{
		CrownSprite->SetRelativeLocation(CrownOffset);
	}
}

void ADCKingBoar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// the dead frame lies on the ground: the crown slides down onto the head
	if (CrownSprite && IsDead())
	{
		CrownSprite->SetRelativeLocation(FMath::VInterpTo(CrownSprite->GetRelativeLocation(), DeadCrownOffset, DeltaSeconds, 8.0f));
	}

	// Aggro automatically if player approaches within guard range
	if (!IsDead() && (GetBoarState() == EDCBoarState::Idle || GetBoarState() == EDCBoarState::Wander))
	{
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			if (FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) <= GuardRadius)
			{
				if (ADCBoarAIController* AI = Cast<ADCBoarAIController>(GetController()))
				{
					AI->BecomeAngry(Player);
				}
			}
		}
	}
}
