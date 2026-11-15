// Director's Cut - Beat 4 King Boar guarding the Princess shed.

#include "DirectorsCut/AI/DCKingBoar.h"
#include "DirectorsCut/AI/DCBoarAIController.h"
#include "DirectorsCut/Combat/DCFloatingHPBar.h"
#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "DirectorsCut/UI/DCBaconBarWidget.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Texture2D.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADCKingBoar::ADCKingBoar()
{
	SetActorScale3D(FVector(2.5f));

	// Crown King Bacon Jr. 120 HP, because Taisho's bacon boss bar has "HP n/120" drawn on it. The player keeps the
	// shotgun from beat 3 on: a full blast is 8 pellets x 25 = 200; at 0.05 it does 10, so about 12 clean blasts
	// (the same fight length as before, when he had 350 HP and took 15%).
	Health->MaxHP = 120.0f;
	Health->DamageTakenScale = 0.05f;
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

	// no small floating bar over him: his health is the bacon boss bar on screen (UDCBaconBarWidget)
	if (HPBar)
	{
		HPBar->bShowWhenFull = false;
		HPBar->SetAutoActivate(false);
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
	// Taisho's crown ("Just The Crown King Bacon Jr", Art/Meshy/JustTheCrown)
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrownArt(TEXT("/Game/DirectorsCut/Characters/Boar/Sprites/T_DC_KingCrown.T_DC_KingCrown"));
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

	if (HPBar)
	{
		HPBar->DestroyComponent();          // the bacon boss bar replaces it
		HPBar = nullptr;
	}

	// the bacon boss bar, top centre, while he is around
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		BaconBar = CreateWidget<UDCBaconBarWidget>(PC, UDCBaconBarWidget::StaticClass());
		if (BaconBar)
		{
			BaconBar->AddToViewport(5);
			if (Health)
			{
				BaconBar->SetHP(Health->GetCurrentHP(), Health->MaxHP);
				Health->OnHealthChanged.AddUniqueDynamic(this, &ADCKingBoar::HandleBaconBarHealth);
			}
		}
	}
}

void ADCKingBoar::HandleBaconBarHealth(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser)
{
	if (BaconBar && HealthComponent)
	{
		BaconBar->SetHP(CurrentHP, HealthComponent->MaxHP);
		if (CurrentHP <= 0.0f)
		{
			// leave the empty bar up a moment, then take it away
			FTimerHandle T;
			GetWorldTimerManager().SetTimer(T, FTimerDelegate::CreateWeakLambda(this, [this]() { RemoveBaconBar(); }), 2.0f, false);
		}
	}
}

void ADCKingBoar::RemoveBaconBar()
{
	if (BaconBar)
	{
		BaconBar->RemoveFromParent();
		BaconBar = nullptr;
	}
}

void ADCKingBoar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveBaconBar();
	Super::EndPlay(EndPlayReason);
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
