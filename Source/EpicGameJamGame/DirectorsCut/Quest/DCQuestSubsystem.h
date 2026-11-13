// Director's Cut - quest tracking (one active quest, driven by gameplay event tags).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCQuestSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDCOnQuestUpdated, FName, Id, int32, Current, int32, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCOnQuestCompleted, FName, Id);

/**
 *  Holds the single active quest. Gameplay code only calls ReportEvent, e.g. a boar calls ReportEvent("Kill.Boar") on death.
 *  The level flow starts and swaps quests; the HUD listens to the delegates.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCQuestSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Starts a new quest, replacing any active one. Each ReportEvent(EventTag) counts one step towards Target. */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void StartQuest(FName Id, FText Title, int32 Target, FName EventTag);

	/** Reports a gameplay event (e.g. "Kill.Boar"). Advances the active quest if the tag matches. */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void ReportEvent(FName EventTag);

	/** Swaps the active quest mid-progress (the Director changing the objective). Optionally carries the current count over. */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void ReplaceQuest(FName Id, FText Title, int32 Target, FName EventTag, bool bKeepProgress = false);

	/** Fired when a quest starts, is replaced, or its progress changes */
	UPROPERTY(BlueprintAssignable, Category="Quest")
	FDCOnQuestUpdated OnQuestUpdated;

	/** Fired once when the active quest reaches its target */
	UPROPERTY(BlueprintAssignable, Category="Quest")
	FDCOnQuestCompleted OnQuestCompleted;

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
