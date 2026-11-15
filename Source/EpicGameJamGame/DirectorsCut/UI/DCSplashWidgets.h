// Rated PG for Pig - the two screens shown once per launch before the main menu:
// the Unreal Engine logo splash, then a plain-text content notice.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCSplashWidgets.generated.h"

class UCanvasPanel;
class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnIntroScreenFinished);

/**
 *  A full-screen black screen that fades its content in, holds it, fades it out, then fires OnFinished.
 *  Any key, mouse click or gamepad button skips it at once (a held key's repeats and presses in the first
 *  0.25 s are ignored, so the press that skipped the screen before never skips this one too).
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCIntroScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCIntroScreenWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Intro", meta=(ClampMin=0, Units="s"))
	float FadeInTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Intro", meta=(ClampMin=0, Units="s"))
	float HoldTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Intro", meta=(ClampMin=0, Units="s"))
	float FadeOutTime = 0.5f;

	/** Fired once, when the screen has faded out or was skipped */
	UPROPERTY(BlueprintAssignable, Category="Intro")
	FDCOnIntroScreenFinished OnFinished;

	/** Ends the screen now (same as a key press) */
	UFUNCTION(BlueprintCallable, Category="Intro")
	void Skip();

	bool IsFinished() const { return bFinished; }
	float GetElapsed() const { return Elapsed; }

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Adds this screen's content to the canvas and returns the widget that fades (the black backdrop does not) */
	virtual UWidget* BuildContent(UCanvasPanel* Canvas) PURE_VIRTUAL(UDCIntroScreenWidget::BuildContent, return nullptr;);

	/** False = nothing to show (for example the logo file is missing): the screen finishes on its first tick */
	virtual bool HasContent() const { return true; }

	void Finish();

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Content;

	float Elapsed = 0.0f;
	bool bFinished = false;
	bool bWarmedUp = false;
	float WarmupTime = 0.0f;
	int32 SmoothFrames = 0;
};

/**
 *  The official Unreal Engine logo, unaltered, at its own aspect ratio, centred on black.
 *  The texture is the file from Epic's branding page imported as /Game/UI/Splash/T_UE_Logo.
 *  If that asset does not exist the splash is skipped: no stand-in is ever drawn.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCSplashWidget : public UDCIntroScreenWidget
{
	GENERATED_BODY()

public:

	UDCSplashWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Splash")
	TSoftObjectPtr<UTexture2D> LogoTexture;

	/** Logo height on a 1080-high screen. The width follows the picture's own aspect; the rest of the screen
	 *  around it is empty black, well over the clear space Epic's guidelines ask for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Splash", meta=(ClampMin=64))
	float LogoHeight = 320.0f;

	/** Epic's trademark notice, shown under the logo (wording from brand.epicgames.com: do not change it) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Splash")
	FText TrademarkNotice;

protected:

	virtual UWidget* BuildContent(UCanvasPanel* Canvas) override;
	virtual bool HasContent() const override { return LoadedLogo != nullptr; }

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LoadedLogo;
};

/**
 *  Content notice: words only, no characters, icons or rating symbols. Shown as a drawn card in the game's style
 *  (CardTexture); the four text lines below are the fallback if the card is missing.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCContentNoticeWidget : public UDCIntroScreenWidget
{
	GENERATED_BODY()

public:

	UDCContentNoticeWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notice")
	FText Heading;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notice")
	FText Line1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notice")
	FText Line2;

	/** Drawn card with the same four lines (Meshy, in the game's style). When set and found it is shown instead of
	 *  the text below; clear it to go back to plain text. A wording change needs a new picture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notice")
	TSoftObjectPtr<UTexture2D> CardTexture;

	/** Card width on a 1920-wide screen (the height follows the picture) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notice", meta=(ClampMin=200))
	float CardWidth = 1150.0f;

	/** The Director's line: smaller, in italics */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Notice")
	FText Quote;

protected:

	virtual UWidget* BuildContent(UCanvasPanel* Canvas) override;
};
