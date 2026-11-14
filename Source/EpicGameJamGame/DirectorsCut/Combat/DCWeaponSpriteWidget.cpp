// Director's Cut - 2D first-person weapon layer.

#include "DirectorsCut/Combat/DCWeaponSpriteWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Engine/Texture2D.h"

TSharedRef<SWidget> UDCWeaponSpriteWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Box = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("WeaponBox"));
		Box->SetStretch(EStretch::ScaleToFit);          // whole frame visible, aspect kept
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("WeaponImage"));
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Box->AddChild(Image);
		if (UScaleBoxSlot* S = Cast<UScaleBoxSlot>(Image->Slot))
		{
			S->SetHorizontalAlignment(HAlign_Right);    // hand sits in the lower-right corner
			S->SetVerticalAlignment(VAlign_Bottom);
		}
		WidgetTree->RootWidget = Box;
	}

	TSharedRef<SWidget> W = Super::RebuildWidget();
	if (CurrentFrame && Image)
	{
		Image->SetBrushFromTexture(CurrentFrame, true);
	}
	return W;
}

void UDCWeaponSpriteWidget::SetFrame(UTexture2D* Frame)
{
	if (Frame == CurrentFrame)
	{
		return;
	}
	CurrentFrame = Frame;
	if (Image && Frame)
	{
		Image->SetBrushFromTexture(Frame, /*bMatchSize*/ true);
	}
}

void UDCWeaponSpriteWidget::SetPlacement(const FVector2D& OffsetPx, float Scale, float AngleDeg)
{
	if (Box)
	{
		FWidgetTransform T;
		T.Translation = OffsetPx;
		T.Scale = FVector2D(Scale, Scale);
		T.Angle = AngleDeg;
		Box->SetRenderTransformPivot(FVector2D(1.0f, 1.0f));   // scale/rotate toward the bottom-right corner
		Box->SetRenderTransform(T);
	}
}
