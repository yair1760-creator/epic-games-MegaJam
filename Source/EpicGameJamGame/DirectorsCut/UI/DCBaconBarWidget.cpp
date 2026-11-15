// Rated PG for Pig - Crown King Bacon Jr's boss bar.

#include "UI/DCBaconBarWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

namespace
{
	// the HP drawn on each of Taisho's pictures, out of 120 (there is no full 120/120 drawing: full health shows 108)
	const int32 GBaconSteps[] = { 108, 96, 84, 72, 60, 48, 24, 12, 0 };
}

TSharedRef<SWidget> UDCBaconBarWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BaconBarCanvas"));
		WidgetTree->RootWidget = Canvas;

		for (int32 Step : GBaconSteps)
		{
			const FString Path = FString::Printf(TEXT("/Game/DirectorsCut/UI/Boss/T_UI_BaconBar_%d.T_UI_BaconBar_%d"), Step, Step);
			if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *Path))
			{
				Steps.Add(Step, T);
			}
		}

		BarImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BaconBar"));
		BarImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(BarImage))
		{
			S->SetAnchors(FAnchors(0.5f, 0.0f));
			S->SetAlignment(FVector2D(0.5f, 0.0f));
			S->SetPosition(FVector2D(0.0f, TopMargin));
			S->SetSize(BarSize);
		}
		if (TObjectPtr<UTexture2D>* First = Steps.Find(108))
		{
			BarImage->SetBrushFromTexture(First->Get(), false);
			ShownStep = 108;
		}
	}
	return Super::RebuildWidget();
}

void UDCBaconBarWidget::SetHP(float CurrentHP, float MaxHP)
{
	const float OnArt = MaxHP > 0.0f ? FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f) * 120.0f : 0.0f;
	int32 Pick = GBaconSteps[0];
	for (int32 Step : GBaconSteps)
	{
		if (Step >= OnArt - KINDA_SMALL_NUMBER)
		{
			Pick = Step;            // steps go down: the last one still at or above the HP is the closest
		}
	}
	if (CurrentHP <= 0.0f)
	{
		Pick = 0;
	}
	else if (Pick == 0)
	{
		Pick = 12;                  // alive: never show an empty bar
	}
	if (Pick != ShownStep && BarImage)
	{
		if (TObjectPtr<UTexture2D>* T = Steps.Find(Pick))
		{
			BarImage->SetBrushFromTexture(T->Get(), false);
			ShownStep = Pick;
		}
	}
}
