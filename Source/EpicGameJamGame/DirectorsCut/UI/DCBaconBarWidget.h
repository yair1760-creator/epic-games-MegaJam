// Rated PG for Pig - Crown King Bacon Jr's boss bar (Taisho's bacon health bar art), top centre of the screen.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCBaconBarWidget.generated.h"

class UImage;
class UTexture2D;

/**
 *  One picture per step: the bar art has "HP n/120" drawn on it (108, 96, 84, 72, 60, 48, 24, 12, 0), so the King
 *  has 120 HP and the bar shows the drawing whose number is the smallest one at or above his current HP.
 *  Built in code; ADCKingBoar adds it when he appears and removes it after he dies.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCBaconBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Bar size on a 1920 x 1080 screen (the art is 530 x 285) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Bar")
	FVector2D BarSize = FVector2D(530.0f, 285.0f);

	/** Distance from the top of the screen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Bar")
	float TopMargin = 10.0f;

	/** Shows the drawing for this HP (out of MaxHP, mapped onto the art's 120) */
	UFUNCTION(BlueprintCallable, Category="Boss Bar")
	void SetHP(float CurrentHP, float MaxHP);

	/** The HP number drawn on the picture now shown (for tests and logs) */
	int32 GetShownStep() const { return ShownStep; }

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BarImage;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UTexture2D>> Steps;

	int32 ShownStep = -1;
};
