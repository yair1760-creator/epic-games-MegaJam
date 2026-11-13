// Director's Cut - HUD subtitles for the Director's lines.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCSubtitlesWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/**
 *  Shows UDCNarratorSubsystem lines: speaker name, typewriter text, optional portrait, fade in/out.
 *  Layout lives in WBP_Subtitles: it needs SpeakerText and LineText; Portrait is optional.
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCSubtitlesWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Reveals the whole current line at once */
	UFUNCTION(BlueprintCallable, Category="Subtitles")
	void SkipTypewriter();

	UFUNCTION(BlueprintPure, Category="Subtitles")
	FString GetSpeakerString() const;

	UFUNCTION(BlueprintPure, Category="Subtitles")
	FString GetLineString() const { return FullText; }

protected:

	UPROPERTY(BlueprintReadOnly, Category="Subtitles", meta=(BindWidget))
	TObjectPtr<UTextBlock> SpeakerText;

	UPROPERTY(BlueprintReadOnly, Category="Subtitles", meta=(BindWidget))
	TObjectPtr<UTextBlock> LineText;

	UPROPERTY(BlueprintReadOnly, Category="Subtitles", meta=(BindWidgetOptional))
	TObjectPtr<UImage> Portrait;

	/** Portrait texture. Leave empty to hide the portrait slot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Subtitles")
	TObjectPtr<UTexture2D> PortraitTexture;

	/** Typewriter speed. Raised automatically so the text is fully shown within TypeTimeFraction of the line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Subtitles", meta=(ClampMin=1))
	float CharsPerSecond = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Subtitles", meta=(ClampMin=0.1, ClampMax=1))
	float TypeTimeFraction = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Subtitles", meta=(ClampMin=0, Units="s"))
	float FadeTime = 0.2f;

	UFUNCTION(BlueprintImplementableEvent, Category="Subtitles")
	void OnSubtitleShown(const FText& Speaker, const FText& Text, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category="Subtitles")
	void OnSubtitleHidden();

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	UFUNCTION()
	void HandleLineStarted(FText Speaker, FText Text, float Duration);

	UFUNCTION()
	void HandleLineFinished(FName RowId);

	FString FullText;
	float RevealedChars = 0.0f;
	float RevealSpeed = 0.0f;
	float Opacity = 0.0f;
	bool bShowing = false;
};
