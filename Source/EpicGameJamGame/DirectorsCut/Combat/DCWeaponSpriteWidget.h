// Director's Cut - 2D first-person weapon drawn on screen (hand + sword frames, classic boomer-shooter style).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCWeaponSpriteWidget.generated.h"

class UImage;
class UScaleBox;
class UTexture2D;

/**
 *  Full-screen layer that shows one weapon frame, scaled to the screen height and pinned to the bottom-right
 *  corner (the frames are drawn as whole screens: hand in the lower right, swings across the view).
 *  Built in code, so no widget Blueprint is needed. ADCSwordWeapon owns it and swaps frames.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCWeaponSpriteWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Shows this frame (keeps the texture's own aspect) */
	void SetFrame(UTexture2D* Frame);

	/** Bob / sway offset in screen pixels (positive Y = down), overall size (1 = frame fills screen height), and optional tilt in degrees */
	void SetPlacement(const FVector2D& OffsetPx, float Scale, float AngleDeg = 0.0f);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY()
	TObjectPtr<UScaleBox> Box;

	UPROPERTY()
	TObjectPtr<UImage> Image;

private:

	TObjectPtr<UTexture2D> CurrentFrame;
};
