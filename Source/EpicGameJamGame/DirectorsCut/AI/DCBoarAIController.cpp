// Director's Cut - boar brain.

#include "DirectorsCut/AI/DCBoarAIController.h"
#include "DirectorsCut/AI/DCBoar.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

ADCBoarAIController::ADCBoarAIController()
{
	bStartAILogicOnPossess = true;
}

ADCBoar* ADCBoarAIController::GetBoar() const
{
	return Cast<ADCBoar>(GetPawn());
}

void ADCBoarAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (ADCBoar* Boar = GetBoar())
	{
		if (UDCHealthComponent* Health = Boar->GetHealth())
		{
			Health->OnHealthChanged.AddUniqueDynamic(this, &ADCBoarAIController::HandleHealthChanged);
			Health->OnDied.AddUniqueDynamic(this, &ADCBoarAIController::HandleDied);
		}

		if (Boar->bAutoAggroPlayer)
		{
			if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
			{
				AngerTarget = Player;
				LastProvokedTime = GetWorld()->GetTimeSeconds();
				GetWorldTimerManager().SetTimer(StateTimer, this, &ADCBoarAIController::StartCharge, 0.15f, false);
				return;
			}
		}
	}

	// small random delay so a group of boars doesn't move in lockstep
	GoIdle(FMath::FRandRange(0.2f, 1.5f));
}

void ADCBoarAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(StateTimer);
	GetWorldTimerManager().ClearTimer(ChaseTimer);
	GetWorldTimerManager().ClearTimer(FleeTimer);
	Super::OnUnPossess();
}

// ---------------------------------------------------------------- calm: idle / wander

void ADCBoarAIController::GoIdle(float Seconds)
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || Boar->GetBoarState() == EDCBoarState::Dead)
	{
		return;
	}

	// set the state first: stopping a move fires OnMoveCompleted, which must not re-enter here
	Boar->SetBoarState(EDCBoarState::Idle);
	StopMovement();
	GetWorldTimerManager().ClearTimer(ChaseTimer);
	GetWorldTimerManager().SetTimer(StateTimer, this, &ADCBoarAIController::StartWander, FMath::Max(Seconds, 0.05f), false);
}

void ADCBoarAIController::StartWander()
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || Boar->GetBoarState() == EDCBoarState::Dead)
	{
		return;
	}

	Boar->GetCharacterMovement()->MaxWalkSpeed = Boar->WanderSpeed;

	const FVector Target = Boar->PickRandomWanderPoint();
	const EPathFollowingRequestResult::Type Result = MoveToLocation(
		Target,
		/*AcceptanceRadius*/ 60.0f,
		/*bStopOnOverlap*/ true,
		/*bUsePathfinding*/ true,
		/*bProjectDestinationToNavigation*/ true,
		/*bCanStrafe*/ false,
		/*FilterClass*/ nullptr,
		/*bAllowPartialPath*/ false);

	if (Result == EPathFollowingRequestResult::RequestSuccessful)
	{
		FailedMoves = 0;
		Boar->SetBoarState(EDCBoarState::Wander);
	}
	else
	{
		// unreachable / already there: try again shortly (back off if it keeps failing)
		++FailedMoves;
		GoIdle(FMath::Min(0.25f * FailedMoves, 2.0f));
	}
}

void ADCBoarAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	const ADCBoar* Boar = GetBoar();
	if (Boar && Boar->GetBoarState() == EDCBoarState::Wander)
	{
		GoIdle(FMath::FRandRange(Boar->IdleTimeMin, FMath::Max(Boar->IdleTimeMin, Boar->IdleTimeMax)));
	}
	// while charging, ChaseTick handles arrival and re-issues the move
}

// ---------------------------------------------------------------- hostile: angry / charge / bite

void ADCBoarAIController::HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser)
{
	if (Delta >= 0.0f || HealthComponent->IsDead())
	{
		return;
	}

	// who hurt us? the causer itself if it is a pawn, otherwise its instigator (projectiles, weapons)
	APawn* Attacker = Cast<APawn>(DamageCauser);
	if (!Attacker && DamageCauser)
	{
		Attacker = DamageCauser->GetInstigator();
	}
	if (!Attacker || Attacker == GetPawn() || Attacker->IsA<ADCBoar>())
	{
		return;
	}

	// badly hurt: run instead of fighting (unless hyperspawning)
	const ADCBoar* Boar = GetBoar();
	if (Boar && !Boar->bAutoAggroPlayer && HealthComponent->GetHPPercent() <= Boar->FleeHPFraction)
	{
		StartFlee(Attacker);
	}
	else
	{
		BecomeAngry(Attacker);
	}
}

// ---------------------------------------------------------------- flee

void ADCBoarAIController::StartFlee(APawn* From)
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || !From || Boar->GetBoarState() == EDCBoarState::Dead)
	{
		return;
	}

	FleeFrom = From;
	FleeStartTime = GetWorld()->GetTimeSeconds();
	AngerTarget.Reset();
	GetWorldTimerManager().ClearTimer(StateTimer);
	GetWorldTimerManager().ClearTimer(ChaseTimer);

	const bool bWasFleeing = Boar->GetBoarState() == EDCBoarState::Flee;
	Boar->SetBoarState(EDCBoarState::Flee);
	Boar->GetCharacterMovement()->MaxWalkSpeed = Boar->FleeSpeed;

	if (!bWasFleeing)
	{
		// a startled squeal-hop away from the hit
		const FVector Away = (Boar->GetActorLocation() - From->GetActorLocation()).GetSafeNormal2D();
		Boar->LaunchCharacter(Away * 300.0f + FVector(0.0f, 0.0f, 200.0f), true, true);
	}

	MoveAwayFrom(From->GetActorLocation());
	GetWorldTimerManager().SetTimer(FleeTimer, this, &ADCBoarAIController::FleeTick, 0.25f, true);
}

bool ADCBoarAIController::MoveAwayFrom(const FVector& Threat)
{
	ADCBoar* Boar = GetBoar();
	if (!Boar)
	{
		return false;
	}

	const FVector Me = Boar->GetActorLocation();
	FVector Away = (Me - Threat).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = Boar->GetActorForwardVector();
	}

	// straight away first, then fan out to the sides (fences and walls block the straight line)
	static const float Angles[] = { 0.0f, 35.0f, -35.0f, 70.0f, -70.0f, 110.0f, -110.0f };
	for (const float Angle : Angles)
	{
		const FVector Dir = Away.RotateAngleAxis(Angle, FVector::UpVector);
		for (const float Dist : { 900.0f, 500.0f })
		{
			const FVector Dest = Me + Dir * Dist;
			if (!Boar->IsInsideWanderArea(Dest))
			{
				continue;
			}
			const EPathFollowingRequestResult::Type R = MoveToLocation(Dest, 80.0f, true, true, true, false, nullptr, false);
			if (R == EPathFollowingRequestResult::RequestSuccessful)
			{
				return true;
			}
		}
	}
	return false;
}

void ADCBoarAIController::FleeTick()
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || Boar->GetBoarState() != EDCBoarState::Flee)
	{
		GetWorldTimerManager().ClearTimer(FleeTimer);
		return;
	}

	const APawn* Threat = FleeFrom.Get();
	const float Dist = Threat ? FVector::Dist2D(Boar->GetActorLocation(), Threat->GetActorLocation()) : BIG_NUMBER;
	const bool bTimeUp = (GetWorld()->GetTimeSeconds() - FleeStartTime) > Boar->MaxFleeTime;

	if (!Threat || Dist >= Boar->SafeDistance || bTimeUp)
	{
		// safe (or exhausted): calm down, still hurt
		GetWorldTimerManager().ClearTimer(FleeTimer);
		FleeFrom.Reset();
		CalmDown();
		return;
	}

	// reached the last flee point but the threat is still close: keep running
	if (GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		MoveAwayFrom(Threat->GetActorLocation());
	}
}

void ADCBoarAIController::BecomeAngry(APawn* Target)
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || !Target || Boar->GetBoarState() == EDCBoarState::Dead)
	{
		return;
	}

	LastProvokedTime = GetWorld()->GetTimeSeconds();

	// already hostile at this target: just refresh the timer (don't restart the snort)
	const EDCBoarState State = Boar->GetBoarState();
	if (AngerTarget.Get() == Target && (State == EDCBoarState::Angry || State == EDCBoarState::Charge || State == EDCBoarState::Bite))
	{
		return;
	}

	AngerTarget = Target;
	Boar->SetBoarState(EDCBoarState::Angry);
	StopMovement();
	GetWorldTimerManager().ClearTimer(ChaseTimer);
	FaceTarget();

	// readable tell: a little hop
	Boar->LaunchCharacter(FVector(0.0f, 0.0f, 260.0f), false, true);

	GetWorldTimerManager().SetTimer(StateTimer, this, &ADCBoarAIController::StartCharge, FMath::Max(Boar->AngryPause, 0.05f), false);
}

void ADCBoarAIController::CalmDown()
{
	AngerTarget.Reset();
	GetWorldTimerManager().ClearTimer(ChaseTimer);
	ClearFocus(EAIFocusPriority::Gameplay);
	if (ADCBoar* Boar = GetBoar())
	{
		Boar->GetCharacterMovement()->MaxWalkSpeed = Boar->WanderSpeed;
		GoIdle(FMath::FRandRange(Boar->IdleTimeMin, FMath::Max(Boar->IdleTimeMin, Boar->IdleTimeMax)));
	}
}

bool ADCBoarAIController::IsTargetValid() const
{
	const APawn* Target = AngerTarget.Get();
	if (!IsValid(Target) || Target->IsActorBeingDestroyed())
	{
		return false;
	}
	// Shooter characters tag themselves "Dead"; DC Health actors report it directly
	if (Target->ActorHasTag(TEXT("Dead")))
	{
		return false;
	}
	if (const UDCHealthComponent* H = UDCHealthComponent::FindHealth(Target))
	{
		return !H->IsDead();
	}
	return true;
}

float ADCBoarAIController::GapToTarget() const
{
	const APawn* Me = GetPawn();
	const APawn* Target = AngerTarget.Get();
	if (!Me || !Target)
	{
		return BIG_NUMBER;
	}
	float Gap = FVector::Dist2D(Me->GetActorLocation(), Target->GetActorLocation());
	if (const ACharacter* C = Cast<ACharacter>(Me))
	{
		Gap -= C->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	if (const ACharacter* C = Cast<ACharacter>(Target))
	{
		Gap -= C->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	return Gap;
}

void ADCBoarAIController::FaceTarget()
{
	APawn* Me = GetPawn();
	const APawn* Target = AngerTarget.Get();
	if (Me && Target)
	{
		const FVector To = Target->GetActorLocation() - Me->GetActorLocation();
		Me->SetActorRotation(FRotator(0.0f, To.Rotation().Yaw, 0.0f));
	}
}

void ADCBoarAIController::StartCharge()
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || Boar->GetBoarState() == EDCBoarState::Dead)
	{
		return;
	}
	if (!IsTargetValid())
	{
		CalmDown();
		return;
	}

	Boar->SetBoarState(EDCBoarState::Charge);
	Boar->GetCharacterMovement()->MaxWalkSpeed = Boar->ChargeSpeed;
	MoveToActor(AngerTarget.Get(), /*AcceptanceRadius*/ FMath::Max(Boar->BiteRange * 0.5f, 10.0f), /*bStopOnOverlap*/ true,
		/*bUsePathfinding*/ true, /*bCanStrafe*/ false, /*FilterClass*/ nullptr, /*bAllowPartialPath*/ true);

	GetWorldTimerManager().SetTimer(ChaseTimer, this, &ADCBoarAIController::ChaseTick, 0.1f, true);
}

void ADCBoarAIController::ChaseTick()
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || Boar->GetBoarState() != EDCBoarState::Charge)
	{
		GetWorldTimerManager().ClearTimer(ChaseTimer);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float Gap = GapToTarget();

	if (!IsTargetValid() || (!Boar->bAutoAggroPlayer && (Gap > Boar->LoseInterestDistance || (Now - LastProvokedTime) > Boar->MaxChaseTime)))
	{
		if (Boar->bAutoAggroPlayer)
		{
			if (APawn* NewPlayer = UGameplayStatics::GetPlayerPawn(this, 0))
			{
				AngerTarget = NewPlayer;
				MoveToActor(NewPlayer, FMath::Max(Boar->BiteRange * 0.5f, 10.0f), true, true, false, nullptr, true);
				return;
			}
		}
		CalmDown();
		return;
	}

	if (Gap <= Boar->BiteRange && (Now - LastBiteTime) >= Boar->BiteCooldown)
	{
		StartBite();
		return;
	}

	// keep pressing in (the path request may have finished or been knocked off by a hit)
	if (GetMoveStatus() == EPathFollowingStatus::Idle && Gap > Boar->BiteRange * 0.5f)
	{
		MoveToActor(AngerTarget.Get(), FMath::Max(Boar->BiteRange * 0.5f, 10.0f), true, true, false, nullptr, true);
	}
}

void ADCBoarAIController::StartBite()
{
	ADCBoar* Boar = GetBoar();
	if (!Boar)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ChaseTimer);
	Boar->SetBoarState(EDCBoarState::Bite);
	StopMovement();
	FaceTarget();
	GetWorldTimerManager().SetTimer(StateTimer, this, &ADCBoarAIController::FinishBite, FMath::Max(Boar->BiteWindup, 0.01f), false);
}

void ADCBoarAIController::FinishBite()
{
	ADCBoar* Boar = GetBoar();
	if (!Boar || Boar->GetBoarState() != EDCBoarState::Bite)
	{
		return;
	}

	LastBiteTime = GetWorld()->GetTimeSeconds();

	// lunge, then hurt the target only if it is still in reach (stepping back dodges the bite)
	FaceTarget();
	Boar->LaunchCharacter(Boar->GetActorForwardVector() * 350.0f + FVector(0.0f, 0.0f, 120.0f), true, true);
	if (IsTargetValid() && GapToTarget() <= Boar->BiteRange * 1.5f)
	{
		UGameplayStatics::ApplyDamage(AngerTarget.Get(), Boar->BiteDamage, this, Boar, nullptr);
	}

	// back to chasing (bite cooldown is checked in ChaseTick)
	StartCharge();
}

// ---------------------------------------------------------------- death (the body itself falls over in ADCBoar)

void ADCBoarAIController::HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser)
{
	GetWorldTimerManager().ClearTimer(StateTimer);
	GetWorldTimerManager().ClearTimer(ChaseTimer);
	GetWorldTimerManager().ClearTimer(FleeTimer);
	AngerTarget.Reset();
	FleeFrom.Reset();
	if (ADCBoar* Boar = GetBoar())
	{
		Boar->SetBoarState(EDCBoarState::Dead);
	}
	StopMovement();
}
