// Director's Cut - In-game Pause menu ("CUT! - ON HOLD").

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCPauseMenuWidget.generated.h"

class UButton;
class UTextBlock;

/**
 *  Minimal film-slate pause menu toggled with ESC or P during gameplay.
 *  Works with WBP_PauseMenu (styled layout) or standalone via a built-in C++ fallback layout.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	UWidget* GetInitialFocusWidget() const;

	/** Refreshes the displayed Beat / Quest status line when opened. */
	UFUNCTION(BlueprintCallable, Category="Pause")
	void RefreshStatus();

	UFUNCTION(BlueprintCallable, Category="Pause")
	void ResumeGame();

	UFUNCTION(BlueprintCallable, Category="Pause")
	void RestartGame();

	UFUNCTION(BlueprintCallable, Category="Pause")
	void QuitGame();

protected:

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ControlsText;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(BlueprintReadOnly, Category="Pause", meta=(BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:

	void BuildFallbackLayout();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
