// Director's Cut - boar character.

#include "DirectorsCut/AI/DCBoar.h"
#include "DirectorsCut/AI/DCBoarAIController.h"
#include "DirectorsCut/AI/DCBoarSubsystem.h"
#include "Quest/DCQuestSubsystem.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Combat/DCFloatingHPBar.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Rendering/DCSpriteFX.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "Engine/Texture2D.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADCBoar::ADCBoar()
{
	DC_DEFAULT_SFX(GruntSound, "S_BoarGrunt");
	DC_DEFAULT_SFX(AngrySound, "S_BoarAggro");
	DC_DEFAULT_SFX(ChargeSound, "S_BoarCharge");
	DC_DEFAULT_SFX(BiteSound, "S_BoarBite");
	DC_DEFAULT_SFX(HurtSound, "S_BoarSqueal");
	DC_DEFAULT_SFX(DeathSound, "S_BoarDeath");
	DeathSpriteFX = ADCFX_BoarPoof::StaticClass();
	PrimaryActorTick.bCanEverTick = true;

	// low, wide capsule
	GetCapsuleComponent()->InitCapsuleSize(45.0f, 45.0f);

	// AI drives it; face the way it walks
	AIControllerClass = ADCBoarAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 420.0f, 0.0f);
	Move->MaxWalkSpeed = WanderSpeed;
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 200.0f;

	Health = CreateDefaultSubobject<UDCHealthComponent>(TEXT("Health"));
	Health->MaxHP = 60.0f;

	// ---- greybox body
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Hide(TEXT("/Game/VerticalSlice/Materials/MI_LD_Trunk.MI_LD_Trunk"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeLocation(FVector(-5.0f, 0.0f, -5.0f));
	BodyMesh->SetRelativeScale3D(FVector(1.1f, 0.6f, 0.6f));     // ~110 x 60 x 60 cm (chamfer cube is 100 cm, pivot centre)

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetRelativeLocation(FVector(62.0f, 0.0f, 0.0f));
	HeadMesh->SetRelativeScale3D(FVector(0.4f, 0.42f, 0.42f));

	if (Cube.Succeeded())
	{
		BodyMesh->SetStaticMesh(Cube.Object);
		HeadMesh->SetStaticMesh(Cube.Object);
	}
	if (Hide.Succeeded())
	{
		BodyMesh->SetMaterial(0, Hide.Object);
		HeadMesh->SetMaterial(0, Hide.Object);
	}

	StateText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("State Text"));
	StateText->SetupAttachment(GetCapsuleComponent());
	StateText->SetRelativeLocation(FVector(0.0f, 0.0f, 112.0f));

	// 2.5D sprite standing on the ground (capsule bottom); placeholder art until the real boar is in
	Sprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Sprite"));
	Sprite->SetupAttachment(GetCapsuleComponent());
	// designer art (front view, 1042 px square, feet ~6% above the bottom edge): sink the card a little so the hooves touch the ground
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -45.0f - 8.0f));
	Sprite->FrontSize = FVector2D(125.0f, 125.0f);
	Sprite->SideSize = FVector2D(165.0f, 110.0f);
	static ConstructorHelpers::FObjectFinder<UTexture2D> PlaceholderFront(TEXT("/Game/DirectorsCut/Rendering/Sprites/T_DC_Boar_Front_Placeholder.T_DC_Boar_Front_Placeholder"));

	// designer frames (front view; used from every angle until side/back views exist)
	#define DC_BOAR_FRAME(Var, Name) static ConstructorHelpers::FObjectFinder<UTexture2D> Var(TEXT("/Game/DirectorsCut/Characters/Boar/Sprites/T_Boar_" Name ".T_Boar_" Name))
	DC_BOAR_FRAME(Idle1, "idle_01");   DC_BOAR_FRAME(Idle2, "idle_02");
	DC_BOAR_FRAME(Run1, "run_01");     DC_BOAR_FRAME(Run2, "run_02");     DC_BOAR_FRAME(Run3, "run_03");
	DC_BOAR_FRAME(Charge1, "charge_01"); DC_BOAR_FRAME(Charge2, "charge_02");
	DC_BOAR_FRAME(Hit1, "hit_01");
	DC_BOAR_FRAME(Fall1, "fall_01");   DC_BOAR_FRAME(Fall2, "fall_02");
	#undef DC_BOAR_FRAME

	auto MakeAnim = [](std::initializer_list<UTexture2D*> Frames, float FPS, bool bLoop, std::initializer_list<float> Durations = {})
	{
		FDCSpriteAnim A;
		for (UTexture2D* T : Frames) { if (T) A.Frames.Add(T); }
		for (float D : Durations) { A.FrameDurations.Add(D); }
		A.FPS = FPS;
		A.bLoop = bLoop;
		return A;
	};
	if (Run1.Succeeded())
	{
		// idle: mostly eyes open, a short eyes-closed blink/doze
		Sprite->Animations.Add(TEXT("Idle"),   MakeAnim({ Idle1.Object, Idle2.Object }, 2.0f, true, { 2.2f, 0.25f }));
		// run cycle from the designer's GIF: 3 frames in ~0.48 s at walking pace (sped up with movement speed)
		Sprite->Animations.Add(TEXT("Run"),    MakeAnim({ Run1.Object, Run2.Object, Run3.Object }, 6.25f, true));
		Sprite->Animations.Add(TEXT("Angry"),  MakeAnim({ Charge1.Object, Charge2.Object }, 5.0f, true));
		Sprite->Animations.Add(TEXT("Charge"), MakeAnim({ Charge1.Object, Charge2.Object }, 8.0f, true));
		Sprite->Animations.Add(TEXT("Hit"),    MakeAnim({ Hit1.Object }, 4.0f, false));            // 0.25 s flinch
		Sprite->Animations.Add(TEXT("Hurt"),   MakeAnim({ Fall1.Object }, 1.0f, true));            // injured, standing (low HP)
		Sprite->Animations.Add(TEXT("Dead"),   MakeAnim({ Fall1.Object, Fall2.Object }, 4.0f, false)); // wobble, then down
		Sprite->Animations.Add(TEXT("Sleep"),  MakeAnim({ Fall2.Object }, 1.0f, true));            // lying down (not used by the AI yet)
		Sprite->StartAnimation = TEXT("Idle");
		Sprite->FrontTexture = Idle1.Object;
	}
	else if (PlaceholderFront.Succeeded())
	{
		Sprite->FrontTexture = PlaceholderFront.Object;
	}

	HPBar = CreateDefaultSubobject<UDCFloatingHPBar>(TEXT("HP Bar"));
	HPBar->SetupAttachment(GetCapsuleComponent());
	HPBar->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
	StateText->SetHorizontalAlignment(EHTA_Center);
	StateText->SetWorldSize(28.0f);
	StateText->SetTextRenderColor(FColor::White);
	StateText->SetUsingAbsoluteRotation(true);

	Tags.Add(TEXT("Boar"));
}

void ADCBoar::BeginPlay()
{
	Super::BeginPlay();

	HomeLocation = GetActorLocation();
	GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
	StateText->SetVisibility(bShowDebugState);
	UpdateStateText();

	// sprite or greybox body
	Sprite->SetVisibility(bUseSprite, true);
	Sprite->SetComponentTickEnabled(bUseSprite);
	BodyMesh->SetVisibility(!bUseSprite);
	HeadMesh->SetVisibility(!bUseSprite);

	Health->OnDied.AddUniqueDynamic(this, &ADCBoar::HandleDied);
	Health->OnHealthChanged.AddUniqueDynamic(this, &ADCBoar::HandleHealthChanged);

	if (UDCBoarSubsystem* Boars = GetWorld()->GetSubsystem<UDCBoarSubsystem>())
	{
		Boars->RegisterBoar(this);
	}

	NextGruntTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(1.0f, GruntInterval.Y);
}

void ADCBoar::PlayBoarSound(USoundBase* Sound, float MinInterval)
{
	// per-boar volume / pitch (the King is louder and deeper); the library adds jitter and stops a swarm stacking one sound
	UDCFXLibrary::PlayWorldSound(this, Sound, GetActorLocation(), SoundVolume, 0.08f, MinInterval, SoundPitch);
}

void ADCBoar::HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser)
{
	// exactly once per boar, at the moment it dies (guards double hits on one frame, Kill Z after death, ...)
	if (bDeathHandled)
	{
		return;
	}
	bDeathHandled = true;

	// quest contract (lead's package): the only thing a boar tells the quest system
	if (!KillEventTag.IsNone())
	{
		if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
		{
			Quest->ReportEvent(KillEventTag);
		}
	}

	SetBoarState(EDCBoarState::Dead);
	PlayBoarSound(DeathSound);
	UDCFXLibrary::SpawnWorldFX(this, DeathFX, GetActorLocation(), FRotator::ZeroRotator, GetActorScale3D().X);
	// raised a little so the bottom puffs don't sink into the ground
	UDCFXLibrary::SpawnSpriteFX(this, DeathSpriteFX, GetActorLocation() + FVector(0.0f, 0.0f, 40.0f * GetActorScale3D().Z), GetActorScale3D().X, 30.0f);

	// sprite: wobble and fall over (the frames hold on the last one)
	if (bUseSprite && Sprite)
	{
		Sprite->PlayAnimation(TEXT("Dead"), 1.0f, true);
	}

	// stop moving, stop blocking the player, keep resting on the ground
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	StateText->SetVisibility(false);
	Tags.Add(TEXT("Dead"));
	SetCanBeDamaged(false);   // swings pass over the body

	// tip over (Tick), then remove the body
	DeathStartRotation = GetActorRotation();
	DeathAnimTime = 0.0f;
	if (CorpseTime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(CorpseTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { Destroy(); }), CorpseTime, false);
	}

	// who did it?
	APawn* KillerPawn = Killer ? Killer->GetPawn() : nullptr;
	if (!KillerPawn)
	{
		KillerPawn = Cast<APawn>(DamageCauser);
	}

	OnBoarDied.Broadcast(this, KillerPawn);
	if (UDCBoarSubsystem* Boars = GetWorld()->GetSubsystem<UDCBoarSubsystem>())
	{
		Boars->NotifyBoarKilled(this, KillerPawn);
	}
	// (meat drops are the lead's ADCBoarMeatPickup, spawned by ADCGameMode)
}

void ADCBoar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateSpriteAnimation();

	// calm grunts now and then
	if ((BoarState == EDCBoarState::Idle || BoarState == EDCBoarState::Wander) && GruntSound && GetWorld()->GetTimeSeconds() >= NextGruntTime)
	{
		PlayBoarSound(GruntSound);
		NextGruntTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(GruntInterval.X, FMath::Max(GruntInterval.X, GruntInterval.Y));
	}

	// death: roll onto its side over 0.35 s (greybox body only; the sprite has its own fall frames)
	if (!bUseSprite && DeathAnimTime >= 0.0f && DeathAnimTime < 1.0f)
	{
		DeathAnimTime = FMath::Min(1.0f, DeathAnimTime + DeltaSeconds / 0.35f);
		const float A = FMath::InterpEaseIn(0.0f, 1.0f, DeathAnimTime, 2.0f);
		SetActorRotation(DeathStartRotation + FRotator(0.0f, 0.0f, 90.0f * A));
	}

	// keep the debug label turned to the player camera
	if (bShowDebugState)
	{
		if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			if (const APawn* ViewPawn = PC->GetPawn())
			{
				CamLoc = ViewPawn->GetPawnViewLocation();
			}
			StateText->SetWorldRotation(FRotator(0.0f, (CamLoc - StateText->GetComponentLocation()).Rotation().Yaw, 0.0f));
		}
	}
}

void ADCBoar::UpdateSpriteAnimation()
{
	if (!bUseSprite || !Sprite || BoarState == EDCBoarState::Dead)
	{
		return;
	}

	const float Speed = GetVelocity().Size2D();
	const bool bMoving = Speed > 20.0f;

	switch (BoarState)
	{
	case EDCBoarState::Angry:
		Sprite->PlayAnimation(TEXT("Angry"));
		break;
	case EDCBoarState::Charge:
	case EDCBoarState::Bite:
		Sprite->PlayAnimation(TEXT("Charge"), FMath::Clamp(Speed / ChargeSpeed, 0.6f, 1.5f));
		break;
	default:
		if (bMoving)
		{
			// legs keep pace with the ground: walking speed = the GIF's speed
			Sprite->PlayAnimation(TEXT("Run"), FMath::Clamp(Speed / FMath::Max(WanderSpeed, 1.0f), 0.6f, 2.5f));
		}
		else if (Health && Health->GetHPPercent() <= FleeHPFraction)
		{
			Sprite->PlayAnimation(TEXT("Hurt"));
		}
		else
		{
			Sprite->PlayAnimation(TEXT("Idle"));
		}
		break;
	}
}

void ADCBoar::HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser)
{
	// flinch on every hit that doesn't kill
	if (Delta < 0.0f && !HealthComponent->IsDead() && bUseSprite && Sprite)
	{
		Sprite->PlayOneShot(TEXT("Hit"));
	}
	if (Delta < 0.0f && !HealthComponent->IsDead())
	{
		PlayBoarSound(HurtSound);
	}
}

void ADCBoar::SetBoarState(EDCBoarState NewState)
{
	if (NewState == BoarState)
	{
		return;
	}
	BoarState = NewState;
	UpdateStateText();
	if (StateText)
	{
		const bool bHostile = NewState == EDCBoarState::Angry || NewState == EDCBoarState::Charge || NewState == EDCBoarState::Bite;
		StateText->SetTextRenderColor(bHostile ? FColor::Red : FColor::White);
	}
	OnStateChanged.Broadcast(this, NewState);

	switch (NewState)
	{
	case EDCBoarState::Angry:  PlayBoarSound(AngrySound); break;
	case EDCBoarState::Charge: PlayBoarSound(ChargeSound); break;
	case EDCBoarState::Bite:   PlayBoarSound(BiteSound); break;
	default: break;
	}
}

void ADCBoar::UpdateStateText()
{
	if (StateText)
	{
		const UEnum* Enum = StaticEnum<EDCBoarState>();
		StateText->SetText(FText::FromString(Enum ? Enum->GetNameStringByValue(static_cast<int64>(BoarState)) : TEXT("?")));
	}
}

bool ADCBoar::IsInsideWanderArea(const FVector& Location) const
{
	if (!WanderArea)
	{
		return true;
	}
	FVector Origin, Extent;
	WanderArea->GetActorBounds(false, Origin, Extent);
	return FMath::Abs(Location.X - Origin.X) <= Extent.X && FMath::Abs(Location.Y - Origin.Y) <= Extent.Y;
}

FVector ADCBoar::PickRandomWanderPoint() const
{
	const FVector From = GetActorLocation();

	for (int32 Try = 0; Try < 12; ++Try)
	{
		FVector P;
		if (WanderArea)
		{
			FVector Origin, Extent;
			WanderArea->GetActorBounds(false, Origin, Extent);
			P = FVector(FMath::FRandRange(Origin.X - Extent.X, Origin.X + Extent.X),
			            FMath::FRandRange(Origin.Y - Extent.Y, Origin.Y + Extent.Y), From.Z);
			// still respect the radius around home if the area is large
			if (FVector::Dist2D(P, HomeLocation) > WanderRadius)
			{
				continue;
			}
		}
		else
		{
			const FVector2D R = FMath::RandPointInCircle(WanderRadius);
			P = HomeLocation + FVector(R.X, R.Y, 0.0f);
			P.Z = From.Z;
		}

		if (FVector::Dist2D(P, From) >= MinWanderStep)
		{
			return P;
		}
	}

	// fallback: head home
	return HomeLocation;
}
