// Director's Cut - full-screen color overlay for the player's hit flash and death tint.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCScreenTintWidget.generated.h"

class UImage;

/**
 *  A full-screen color wash drawn by UMG (built in code, no widget asset needed).
 *  Used instead of the camera fade so the Director's own fades ("CUT!", beat transitions) are never overridden.
 *  Sits above the 2D sword (z -10) and below the HUD (z 0).
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCScreenTintWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Color with its strength in Color.A (0 = invisible) */
	void SetTint(const FLinearColor& Color);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

private:

	UPROPERTY()
	TObjectPtr<UImage> Wash;

	FLinearColor CurrentTint = FLinearColor::Transparent;
};
