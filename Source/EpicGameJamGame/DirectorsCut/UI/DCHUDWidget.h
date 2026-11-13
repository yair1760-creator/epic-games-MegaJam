// Director's Cut - root HUD widget.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCHUDWidget.generated.h"

class UDCHealthBarWidget;
class UDCQuestTrackerWidget;
class UDCSubtitlesWidget;

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

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidget))
	TObjectPtr<UDCSubtitlesWidget> Subtitles;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> Crosshair;
};
