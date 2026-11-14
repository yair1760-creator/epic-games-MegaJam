// Director's Cut - Beat 5 Victory / "THAT'S A WRAP!" end screen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCVictoryWidget.generated.h"

class UButton;
class UTextBlock;

/**
 *  Shown after defeating The Director in Beat 5.
 *  Works with WBP_CutScreen (Designer 1 styling) or standalone via a built-in C++ fallback layout.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCVictoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCVictoryWidget(const FObjectInitializer& ObjectInitializer);

	/** Populates the wrap screen with run stats and begins the fade-in. */
	UFUNCTION(BlueprintCallable, Category="Victory")
	void SetupVictory(int32 BoarsKilled, float ElapsedSeconds);

	UWidget* GetInitialFocusWidget() const;

	UFUNCTION(BlueprintPure, Category="Victory")
	FString GetTitleString() const;

	UFUNCTION(BlueprintPure, Category="Victory")
	FString GetStatsString() const;

	UFUNCTION(BlueprintCallable, Category="Victory")
	void RestartGame();

	UFUNCTION(BlueprintCallable, Category="Victory")
	void QuitGame();

protected:

	UPROPERTY(BlueprintReadOnly, Category="Victory", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category="Victory", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category="Victory", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ReviewText;

	UPROPERTY(BlueprintReadOnly, Category="Victory", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(BlueprintReadOnly, Category="Victory", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PlayAgainButton;

	UPROPERTY(BlueprintReadOnly, Category="Victory", meta=(BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Victory", meta=(ClampMin=0.05, Units="s"))
	float FadeInDuration = 0.55f;

	UFUNCTION(BlueprintImplementableEvent, Category="Victory")
	void OnVictoryShown(int32 BoarsKilled, float ElapsedSeconds);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	void BuildFallbackLayout();

	UFUNCTION()
	void HandlePlayAgainClicked();

	UFUNCTION()
	void HandleQuitClicked();

	int32 CachedBoarsKilled = 0;
	float CachedElapsedSeconds = 0.0f;
	float CurrentOpacity = 0.0f;
	bool bFadingIn = false;
};
