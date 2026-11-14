// Director's Cut - on-screen controls prompt (first seconds of the game, and again when the shotgun arrives).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCControlsHintWidget.generated.h"

class UVerticalBox;

/**
 *  A small "CONTROLS" card on the right edge of the screen, in the HUD's ink-and-cream look. Built in code (no widget
 *  Blueprint, nothing inside WBP_DCHUD). ADCPlayerController shows it for a few seconds; it fades out and removes itself.
 *  The keys listed are the ones really bound: IMC_Default / IMC_Weapons / IMC_MouseLook and the controller's own keys.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCControlsHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCControlsHintWidget(const FObjectInitializer& ObjectInitializer);

	/** Fills the card (sword set or shotgun set) and starts the countdown */
	UFUNCTION(BlueprintCallable, Category="Controls")
	void ShowFor(bool bShotgun, float Seconds);

	/** Seconds left before the card is gone (0 when finished) */
	UFUNCTION(BlueprintPure, Category="Controls")
	float GetRemaining() const { return Remaining; }

	/** The rows on the card as "KEY|ACTION" strings, for tests */
	UFUNCTION(BlueprintPure, Category="Controls")
	TArray<FString> GetRows() const { return Rows; }

	/** Fade-out time at the end of the countdown */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Controls", meta=(ClampMin=0.0, Units="s"))
	float FadeOutSeconds = 0.4f;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Column;

private:

	void BuildRows();

	TArray<FString> Rows;
	bool bShotgunSet = false;
	float Remaining = 0.0f;
};
