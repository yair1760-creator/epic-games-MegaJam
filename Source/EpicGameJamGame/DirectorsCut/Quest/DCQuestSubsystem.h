// Director's Cut - quest tracking (one active quest, driven by gameplay event tags).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCQuestSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDCOnQuestUpdated, FName, Id, int32, Current, int32, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCOnQuestCompleted, FName, Id);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCOnQuestReplaced, FName, OldId, FName, NewId);

/**
 *  Holds the single active quest. Gameplay code only calls ReportEvent, e.g. a boar calls ReportEvent("Kill.Boar") on death.
 *  The level flow starts and swaps quests; the HUD listens to the delegates.
 *  Console: DC.StartQuest <Id> <Target> <EventTag> [Title...], DC.ReportEvent <EventTag> [Count]
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

	/** Immediately finishes the active quest (used by DC.CompleteQuest for testing). */
	UFUNCTION(BlueprintCallable, Category="Quest")
	void CompleteActiveQuest();

	UFUNCTION(BlueprintPure, Category="Quest")
	bool HasActiveQuest() const { return !ActiveId.IsNone(); }

	UFUNCTION(BlueprintPure, Category="Quest")
	FName GetActiveQuestId() const { return ActiveId; }

	UFUNCTION(BlueprintPure, Category="Quest")
	FText GetActiveQuestTitle() const { return ActiveTitle; }

	UFUNCTION(BlueprintPure, Category="Quest")
	FName GetActiveEventTag() const { return ActiveEventTag; }

	UFUNCTION(BlueprintPure, Category="Quest")
	int32 GetCurrent() const { return Current; }

	UFUNCTION(BlueprintPure, Category="Quest")
	int32 GetTarget() const { return Target; }

	UFUNCTION(BlueprintPure, Category="Quest")
	bool IsActiveQuestComplete() const { return bCompleted; }

	/** Fired when a quest starts, is replaced, or its progress changes */
	UPROPERTY(BlueprintAssignable, Category="Quest")
	FDCOnQuestUpdated OnQuestUpdated;

	/** Fired once when the active quest reaches its target */
	UPROPERTY(BlueprintAssignable, Category="Quest")
	FDCOnQuestCompleted OnQuestCompleted;

	/** Fired by ReplaceQuest before the matching OnQuestUpdated (drives the "quest updated" flash) */
	UPROPERTY(BlueprintAssignable, Category="Quest")
	FDCOnQuestReplaced OnQuestReplaced;

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	void SetQuest(FName Id, const FText& Title, int32 InTarget, FName EventTag, int32 StartCount);
	void BroadcastProgress();

	FName ActiveId;
	FText ActiveTitle;
	FName ActiveEventTag;
	int32 Current = 0;
	int32 Target = 0;
	bool bCompleted = false;
};
