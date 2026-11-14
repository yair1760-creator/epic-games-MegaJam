// Director's Cut - the hero's drawn two-pose death.

#include "UI/DCPlayerDeathSpriteWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

UDCPlayerDeathSpriteWidget::UDCPlayerDeathSpriteWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameDying(TEXT("/Game/DirectorsCut/UI/Death/T_Player_Death_01_Dying.T_Player_Death_01_Dying"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> FrameDead(TEXT("/Game/DirectorsCut/UI/Death/T_Player_Death_02_Dead.T_Player_Death_02_Dead"));
	if (FrameDying.Succeeded()) { SpriteDying = FrameDying.Object; }
	if (FrameDead.Succeeded()) { SpriteDead = FrameDead.Object; }

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UDCPlayerDeathSpriteWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// The whole 1920x1080 canvas, fitted to the screen with its aspect kept and centred: both drawings get
		// the same anchor and scale, so the floor line stays put across the cut.
		Box = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("DeathBox"));
		Box->SetStretch(EStretch::ScaleToFit);
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeathImage"));
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Box->AddChild(Image);
		if (UScaleBoxSlot* S = Cast<UScaleBoxSlot>(Image->Slot))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetVerticalAlignment(VAlign_Center);
		}
		// one transform for the whole canvas, so both drawings keep the same anchor and scale
		Box->SetRenderTransformPivot(CanvasPivot);
		Box->SetRenderScale(FVector2D(CanvasScale, CanvasScale));
		WidgetTree->RootWidget = Box;
	}

	TSharedRef<SWidget> W = Super::RebuildWidget();
	ShownPose = NAME_None;
	ApplyPose();
	return W;
}

void UDCPlayerDeathSpriteWidget::Play()
{
	// art imported after this class was first loaded: pick it up now
	if (!SpriteDying) { SpriteDying = LoadObject<UTexture2D>(nullptr, TEXT("/Game/DirectorsCut/UI/Death/T_Player_Death_01_Dying.T_Player_Death_01_Dying")); }
	if (!SpriteDead) { SpriteDead = LoadObject<UTexture2D>(nullptr, TEXT("/Game/DirectorsCut/UI/Death/T_Player_Death_02_Dead.T_Player_Death_02_Dead")); }

	Elapsed = 0.0f;
	CurrentPose = FName("Dying");
	ShownPose = NAME_None;
	ApplyPose();
}

void UDCPlayerDeathSpriteWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Elapsed += InDeltaTime;

	// 12 fps grid: Dying for DyingTicks, then Dead for as long as the layer is up. Hard cut, nothing in between.
	const int32 AnimTick = FMath::FloorToInt(Elapsed * FMath::Max(AnimFPS, 1.0f));
	CurrentPose = (AnimTick < DyingTicks) ? FName("Dying") : FName("Dead");
	ApplyPose();
}

void UDCPlayerDeathSpriteWidget::ApplyPose()
{
	if (!Image || CurrentPose == ShownPose)
	{
		return;
	}

	UTexture2D* Frame = (CurrentPose == FName("Dead")) ? SpriteDead.Get() : SpriteDying.Get();
	if (Frame)
	{
		Image->SetBrushFromTexture(Frame, /*bMatchSize*/ true);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Image->SetVisibility(ESlateVisibility::Collapsed);     // art missing: show nothing rather than a white box
	}
	ShownPose = CurrentPose;
}
