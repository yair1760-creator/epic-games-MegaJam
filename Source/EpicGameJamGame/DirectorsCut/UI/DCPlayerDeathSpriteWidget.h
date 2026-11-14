// Director's Cut - the hero's drawn two-pose death (clutches his chest, then lies on his side).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCPlayerDeathSpriteWidget.generated.h"

class UImage;
class UScaleBox;
class UTexture2D;

/**
 *  Full-screen layer that plays the player's death once on the 12 fps timing grid:
 *  Dying for 3 ticks (250 ms), then a hard cut to Dead, held until the layer is removed (respawn / beat change).
 *  Both drawings are 1920x1080 canvases with an aligned floor line, so they share one anchor and scale:
 *  the whole canvas is fitted to the screen with its aspect kept. No tweening, no fades.
 *  Built in code; ADCGameMode shows it with the death slate and removes it with it.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCPlayerDeathSpriteWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCPlayerDeathSpriteWidget(const FObjectInitializer& ObjectInitializer);

	/** Starts the sequence from its first drawing (call on the frame the player dies) */
	UFUNCTION(BlueprintCallable, Category="Death")
	void Play();

	/** Current drawing for UI / tests: Dying or Dead */
	UFUNCTION(BlueprintPure, Category="Death")
	FName GetPose() const { return CurrentPose; }

	/** Seconds since Play() */
	UFUNCTION(BlueprintPure, Category="Death")
	float GetElapsed() const { return Elapsed; }

	/** First drawing: the hero clutching his chest (Art/Player/Death/Player_Death_01_Dying.png) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death")
	TObjectPtr<UTexture2D> SpriteDying;

	/** Second drawing: the hero lying on his side (Art/Player/Death/Player_Death_02_Dead.png) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death")
	TObjectPtr<UTexture2D> SpriteDead;

	/** Timing grid in ticks per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death", meta=(ClampMin=1))
	float AnimFPS = 12.0f;

	/** Ticks the first drawing is held (3 ticks = 250 ms) before the hard cut to the second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death", meta=(ClampMin=1))
	int32 DyingTicks = 3;

	/** Size of the canvas on screen (1 = fitted to the screen). Both drawings always share it. 0.9 with the top-centre
	 *  pivot below lifts the floor line clear of the Director's subtitle box, which sits over the bottom of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death", meta=(ClampMin=0.1))
	float CanvasScale = 0.9f;

	/** Point of the canvas that stays put when CanvasScale changes (0.5, 0 = top centre) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Death")
	FVector2D CanvasPivot = FVector2D(0.5f, 0.0f);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	TObjectPtr<UScaleBox> Box;

	UPROPERTY()
	TObjectPtr<UImage> Image;

private:

	void ApplyPose();

	float Elapsed = 0.0f;
	FName CurrentPose = FName("Dying");
	FName ShownPose = NAME_None;
};
