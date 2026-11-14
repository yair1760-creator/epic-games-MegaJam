// Director's Cut - root HUD widget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCHUDWidget.generated.h"

class UDCAmmoCounterWidget;
class UDCHealthBarWidget;
class UDCQuestTrackerWidget;
class UDCSubtitlesWidget;
class UTextBlock;

/**
 *  The single HUD added to the screen by ADCGameMode. WBP_DCHUD holds one instance of each HUD part;
 *  position and style them freely there, but keep the names.
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidget))
	TObjectPtr<UDCQuestTrackerWidget> QuestTracker;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidget))
	TObjectPtr<UDCHealthBarWidget> HealthBar;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UDCAmmoCounterWidget> AmmoCounter;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidget))
	TObjectPtr<UDCSubtitlesWidget> Subtitles;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Crosshair;

	/** Optional top-left viewfinder slate label ("REC | SCENE 1: BOAR HUNT") */
	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TakeText;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UFUNCTION()
	void HandleBeatChanged(int32 Beat);

public:

	void RefreshTakeLabel(int32 Beat);
	void SetCrosshairVisible(bool bVisible);

	UWidget* GetCrosshair() const { return Crosshair; }
	UDCQuestTrackerWidget* GetQuestTracker() const { return QuestTracker; }
	UDCHealthBarWidget* GetHealthBar() const { return HealthBar; }
	UDCAmmoCounterWidget* GetAmmoCounter() const { return AmmoCounter; }
	UDCSubtitlesWidget* GetSubtitles() const { return Subtitles; }
};
