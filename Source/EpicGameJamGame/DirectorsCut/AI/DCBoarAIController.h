// Director's Cut - boar brain. Small state machine on timers (no Behavior Tree / StateTree assets needed).

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "DCBoarAIController.generated.h"

class ADCBoar;
class UDCHealthComponent;

/**
 *  States:
 *  - Idle <-> Wander (system 4): random reachable points inside the wander area.
 *  - Angry -> Charge -> Bite (system 5): when hit by a pawn, it stops and snorts (Angry), runs at the
 *    attacker (Charge) and bites when close (Bite: wind-up, then damage if still in reach). Calms down
 *    and wanders again when the target dies, gets too far away, or after MaxChaseTime without new hits.
 *  - Dead: stops everything (system 6 adds flee + proper death).
 */
UCLASS()
class EPICGAMEJAMGAME_API ADCBoarAIController : public AAIController
{
	GENERATED_BODY()

public:

	ADCBoarAIController();

	UFUNCTION(BlueprintPure, Category="Boar")
	ADCBoar* GetBoar() const;

	/** Who the boar is angry at (null when calm) */
	UFUNCTION(BlueprintPure, Category="Boar")
	APawn* GetAngerTarget() const { return AngerTarget.Get(); }

	/** Stops whatever the boar is doing and goes idle, then wanders */
	UFUNCTION(BlueprintCallable, Category="Boar")
	void GoIdle(float Seconds);

	/** Starts walking to a new random point */
	UFUNCTION(BlueprintCallable, Category="Boar")
	void StartWander();

	/** Makes the boar angry at a pawn (also called automatically when it is hit) */
	UFUNCTION(BlueprintCallable, Category="Boar")
	void BecomeAngry(APawn* Target);

	/** Forgets the target and goes back to wandering */
	UFUNCTION(BlueprintCallable, Category="Boar")
	void CalmDown();

	/** Runs away from a pawn (also called automatically when a hit leaves HP low) */
	UFUNCTION(BlueprintCallable, Category="Boar")
	void StartFlee(APawn* From);

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UFUNCTION()
	void HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser);

private:

	void StartCharge();
	void ChaseTick();
	void StartBite();
	void FinishBite();
	void FleeTick();
	bool MoveAwayFrom(const FVector& Threat);

	TWeakObjectPtr<APawn> FleeFrom;
	FTimerHandle FleeTimer;
	float FleeStartTime = 0.0f;

	bool IsTargetValid() const;
	float GapToTarget() const;
	void FaceTarget();

	TWeakObjectPtr<APawn> AngerTarget;

	FTimerHandle StateTimer;   // idle / angry pause / bite wind-up
	FTimerHandle ChaseTimer;   // range checks while charging
	float LastProvokedTime = -1000.0f;
	float LastBiteTime = -1000.0f;
	int32 FailedMoves = 0;
};
