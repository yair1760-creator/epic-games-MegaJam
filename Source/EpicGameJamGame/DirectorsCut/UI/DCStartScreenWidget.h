// Director's Cut - Title / Start Slate overlay shown on launch before Beat 1 begins.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCStartScreenWidget.generated.h"

class UButton;
class UTextBlock;

/**
 *  Cinema clapperboard start overlay ("DIRECTOR'S CUT - SCENE 1 / TAKE 1").
 *  Works with WBP_StartScreen (styled layout) or standalone via a built-in C++ fallback layout.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCStartScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCStartScreenWidget(const FObjectInitializer& ObjectInitializer);

	UWidget* GetInitialFocusWidget() const;

	UFUNCTION(BlueprintCallable, Category="Start")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category="Start")
	void QuitGame();

protected:

	UPROPERTY(BlueprintReadOnly, Category="Start", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, Category="Start", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, Category="Start", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ControlsText;

	UPROPERTY(BlueprintReadOnly, Category="Start", meta=(BindWidgetOptional))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(BlueprintReadOnly, Category="Start", meta=(BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:

	void BuildFallbackLayout();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
