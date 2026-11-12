// Director's Cut - checkpoint tracking.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CheckpointSubsystem.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCheckpointActivated, int32, Index, AActor*, Trigger);

/** One checkpoint gathered from the level. */
USTRUCT(BlueprintType)
struct FDirectorCheckpoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Checkpoint")
	int32 Index = INDEX_NONE;

	/** Trigger volume that activates this checkpoint (may be null if only a respawn point was found) */
	UPROPERTY(BlueprintReadOnly, Category="Checkpoint")
	TObjectPtr<AActor> Trigger;

	/** Where the player respawns (respawn point if present, otherwise the trigger) */
	UPROPERTY(BlueprintReadOnly, Category="Checkpoint")
	FTransform RespawnTransform;
};

/**
 *  Builds the checkpoint list from level actors at BeginPlay, with no extra classes to place.
 *  - Trigger:  any actor with tags "Checkpoint" + "CP_<NN>" and overlap collision (e.g. a TriggerBox).
 *  - Respawn:  any actor with tags "Checkpoint" + "CP_<NN>" + "Respawn" (e.g. a TargetPoint). Yaw is used as facing.
 *  Checkpoints only progress forward. The lowest index is active from the start.
 *  Console: DC.GotoCheckpoint <N>  (teleports the local player and activates it).
 */
UCLASS()
class EPICGAMEJAMGAME_API UCheckpointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Activates a checkpoint. Ignored if it is behind the current one unless bForce. Returns true if it became active. */
	UFUNCTION(BlueprintCallable, Category="Checkpoint")
	bool ActivateCheckpoint(int32 Index, bool bForce = false);

	UFUNCTION(BlueprintPure, Category="Checkpoint")
	int32 GetActiveCheckpointIndex() const { return ActiveIndex; }

	UFUNCTION(BlueprintPure, Category="Checkpoint")
	bool HasActiveCheckpoint() const { return Checkpoints.Contains(ActiveIndex); }

	/** Respawn transform of the active checkpoint (identity if none) */
	UFUNCTION(BlueprintPure, Category="Checkpoint")
	FTransform GetRespawnTransform() const;

	/** Sorted indices of all checkpoints found in the level */
	UFUNCTION(BlueprintPure, Category="Checkpoint")
	TArray<int32> GetCheckpointIndices() const;

	/** Moves the player's pawn to a checkpoint and makes it active (debug / level select) */
	UFUNCTION(BlueprintCallable, Category="Checkpoint")
	bool TeleportPlayerToCheckpoint(APlayerController* PlayerController, int32 Index);

	/** Fired when a new checkpoint becomes active (hook the Narrator / HUD here) */
	UPROPERTY(BlueprintAssignable, Category="Checkpoint")
	FOnCheckpointActivated OnCheckpointActivated;

	static const FName CheckpointTag;
	static const FName RespawnTag;

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	UFUNCTION()
	void HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor);

	static int32 ParseIndex(const AActor* Actor);

	UPROPERTY()
	TMap<int32, FDirectorCheckpoint> Checkpoints;

	int32 ActiveIndex = INDEX_NONE;
};
