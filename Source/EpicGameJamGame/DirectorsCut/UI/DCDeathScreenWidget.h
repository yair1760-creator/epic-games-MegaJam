// Director's Cut - "CUT! TAKE 2" Death / Respawn Slate overlay.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCDeathScreenWidget.generated.h"

class UTextBlock;

/**
 *  Non-interactive cinema clapperboard overlay shown during the player's respawn delay.
 *  Works with WBP_DeathScreen (styled layout) or standalone via a built-in C++ fallback layout.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCDeathScreenWidget(const FObjectInitializer& ObjectInitializer);

	/** Populates the slate for this take and starts the respawn countdown. */
	UFUNCTION(BlueprintCallable, Category="Death")
	void SetupDeath(int32 InTakeNumber, const FString& SceneName, float RespawnDuration);

	UFUNCTION(BlueprintPure, Category="Death")
	int32 GetTakeNumber() const { return TakeNumber; }

	UFUNCTION(BlueprintPure, Category="Death")
	FString GetTitleString() const;

	UFUNCTION(BlueprintPure, Category="Death")
	FString GetSubtitleString() const;

	UFUNCTION(BlueprintPure, Category="Death")
	FString GetCountdownString() const;

protected:

	UPROPERTY(BlueprintReadOnly, Category="Death", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category="Death", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category="Death", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CountdownText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Death", meta=(ClampMin=0.05, Units="s"))
	float FadeInDuration = 0.18f;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void BuildFallbackLayout();
	void RefreshCountdownText();

	int32 TakeNumber = 2;
	float RemainingSeconds = 5.0f;
	float CurrentOpacity = 0.0f;
	bool bFadingIn = false;
};
