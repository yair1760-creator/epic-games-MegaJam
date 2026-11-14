// Director's Cut - Beat 4 King Boar guarding the Princess shed.

#include "DirectorsCut/AI/DCKingBoar.h"
#include "DirectorsCut/AI/DCBoarAIController.h"
#include "DirectorsCut/Combat/DCFloatingHPBar.h"
#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADCKingBoar::ADCKingBoar()
{
	SetActorScale3D(FVector(2.5f));

	Health->MaxHP = 350.0f;   // sword (25 dmg) needs 14 hits
	FleeHPFraction = 0.0f;
	AngryPause = 0.35f;
	ChargeSpeed = 740.0f;
	BiteDamage = 25.0f;
	BiteRange = 115.0f;
	BiteCooldown = 0.85f;
	LoseInterestDistance = 6000.0f;
	MaxChaseTime = 60.0f;
	WanderRadius = 700.0f;
	KillEventTag = FName("Kill.KingBoar");
	CorpseTime = 5.0f;
	SoundVolume = 0.25f;   // same boar sounds, bigger and deeper
	SoundPitch = 0.7f;

	if (HPBar)
	{
		HPBar->bShowWhenFull = true;
		HPBar->Width = 110.0f;
		HPBar->Height = 12.0f;
		HPBar->SetRelativeLocation(FVector(0.0f, 0.0f, 118.0f));
	}

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

	// Enrage below 50% HP: faster royal rush + red-gold tint
	if (!bEnraged && !IsDead() && Health && Health->GetHPPercent() <= 0.5f)
	{
		bEnraged = true;
		ChargeSpeed = EnragedChargeSpeed;
		if (GetBoarState() == EDCBoarState::Charge || GetBoarState() == EDCBoarState::Angry)
		{
			GetCharacterMovement()->MaxWalkSpeed = ChargeSpeed;
		}
		if (Sprite)
		{
			Sprite->SetTint(FLinearColor(1.45f, 0.55f, 0.35f), KingBrightness * 1.15f);
		}
		PlayBoarSound(AngrySound);
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
