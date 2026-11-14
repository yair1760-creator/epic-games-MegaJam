// Director's Cut - full-screen color overlay.

#include "DirectorsCut/Combat/DCScreenTintWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"

TSharedRef<SWidget> UDCScreenTintWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		Wash = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Wash"));
		Wash->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Wash;   // the root of a viewport widget fills the whole screen
	}
	TSharedRef<SWidget> W = Super::RebuildWidget();
	if (Wash)
	{
		Wash->SetColorAndOpacity(CurrentTint);
	}
	return W;
}

void UDCScreenTintWidget::SetTint(const FLinearColor& Color)
{
	CurrentTint = Color;
	if (Wash)
	{
		Wash->SetColorAndOpacity(Color);
	}
	SetVisibility(Color.A > 0.001f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
