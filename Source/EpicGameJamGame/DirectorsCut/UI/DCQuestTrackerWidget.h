// Director's Cut - HUD quest tracker ("Boars killed: 3/10").

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCQuestTrackerWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/**
 *  Shows the active quest of UDCQuestSubsystem. Hidden while there is no quest.
 *  Layout lives in WBP_QuestTracker: it needs TitleText and CountText; the animations are optional.
 *  Without animations a simple code-driven bump (count change) and blink (quest replaced) are used.
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Quest title, e.g. "Boars killed" */
	UPROPERTY(BlueprintReadOnly, Category="Quest", meta=(BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	/** Progress, e.g. "3/10" */
	UPROPERTY(BlueprintReadOnly, Category="Quest", meta=(BindWidget))
	TObjectPtr<UTextBlock> CountText;

	/** Played when the count changes */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Quest", meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> BumpAnim;

	/** Played once when the quest completes */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Quest", meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> CompleteAnim;

	/** Played when the Director swaps the quest */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Quest", meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> QuestUpdatedAnim;

	/** Count text format. {Current} and {Target} are replaced. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Quest")
	FText CountFormat = NSLOCTEXT("DirectorsCut", "QuestCountFormat", "{Current}/{Target}");

	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnProgressChanged(int32 Current, int32 Target);

	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnQuestComplete();

	UFUNCTION(BlueprintImplementableEvent, Category="Quest")
	void OnQuestSwapped();

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	UFUNCTION()
	void HandleQuestUpdated(FName Id, int32 Current, int32 Target);

	UFUNCTION()
	void HandleQuestCompleted(FName Id);

	UFUNCTION()
	void HandleQuestReplaced(FName OldId, FName NewId);

	void Refresh();

	int32 LastCount = INDEX_NONE;
	float BumpTime = 0.0f;
	float BlinkTime = 0.0f;
};
