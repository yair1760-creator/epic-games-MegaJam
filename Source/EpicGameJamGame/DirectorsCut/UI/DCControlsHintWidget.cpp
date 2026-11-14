// Director's Cut - on-screen controls prompt.

#include "UI/DCControlsHintWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"

namespace
{
	const FLinearColor InkPanel(0.07f, 0.055f, 0.065f, 0.90f);
	const FLinearColor Gold(0.95f, 0.78f, 0.22f, 1.0f);
	const FLinearColor Cream(0.96f, 0.93f, 0.86f, 1.0f);

	FSlateFontInfo HudFont(const TCHAR* Path, int32 Size, const FSlateFontInfo& Fallback)
	{
		if (UObject* FontObj = StaticLoadObject(UFont::StaticClass(), nullptr, Path, nullptr, LOAD_NoWarn))
		{
			return FSlateFontInfo(FontObj, Size);
		}
		FSlateFontInfo F = Fallback;
		F.Size = Size;
		return F;
	}
}

UDCControlsHintWidget::UDCControlsHintWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UDCControlsHintWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ControlsRoot"));
		WidgetTree->RootWidget = Root;

		UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ControlsCard"));
		Card->SetBrushColor(InkPanel);
		Card->SetPadding(FMargin(22.0f, 14.0f, 22.0f, 16.0f));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Card))
		{
			S->SetAnchors(FAnchors(1.0f, 0.5f, 1.0f, 0.5f));      // right edge, vertically centred: clear of the HUD corners
			S->SetAlignment(FVector2D(1.0f, 0.5f));
			S->SetPosition(FVector2D(-32.0f, 0.0f));
			S->SetAutoSize(true);
		}

		Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsColumn"));
		Card->SetContent(Column);
		BuildRows();
	}
	return Super::RebuildWidget();
}

void UDCControlsHintWidget::BuildRows()
{
	if (!Column || !WidgetTree)
	{
		return;
	}
	Column->ClearChildren();
	Rows.Reset();

	UTextBlock* Probe = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	const FSlateFontInfo Fallback = Probe->GetFont();
	const FSlateFontInfo HeadFont = HudFont(TEXT("/Game/DirectorsCut/UI/Fonts/Font_Mono.Font_Mono"), 11, Fallback);
	const FSlateFontInfo KeyFont = HudFont(TEXT("/Game/DirectorsCut/UI/Fonts/Font_Condensed.Font_Condensed"), 17, Fallback);
	const FSlateFontInfo ActFont = HudFont(TEXT("/Game/DirectorsCut/UI/Fonts/Font_Body.Font_Body"), 15, Fallback);

	UTextBlock* Head = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsHeader"));
	Head->SetText(FText::FromString(bShotgunSet ? TEXT("NEW PROP  |  SHOTGUN") : TEXT("STAGE DIRECTIONS  |  CONTROLS")));
	Head->SetFont(HeadFont);
	Head->SetColorAndOpacity(FSlateColor(Gold));
	if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Head))
	{
		S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	auto AddRow = [&](const TCHAR* Key, const TCHAR* Action)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		USizeBox* KeyBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		KeyBox->SetWidthOverride(150.0f);
		UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		KeyText->SetText(FText::FromString(Key));
		KeyText->SetFont(KeyFont);
		KeyText->SetColorAndOpacity(FSlateColor(Gold));
		KeyBox->SetContent(KeyText);
		Row->AddChildToHorizontalBox(KeyBox);

		UTextBlock* ActText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ActText->SetText(FText::FromString(Action));
		ActText->SetFont(ActFont);
		ActText->SetColorAndOpacity(FSlateColor(Cream));
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(ActText))
		{
			S->SetVerticalAlignment(VAlign_Center);
		}

		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(Row))
		{
			S->SetPadding(FMargin(0.0f, 2.0f));
		}
		Rows.Add(FString::Printf(TEXT("%s|%s"), Key, Action));
	};

	if (bShotgunSet)
	{
		AddRow(TEXT("LEFT MOUSE"), TEXT("Fire"));
		AddRow(TEXT("R"), TEXT("Reload"));
	}
	else
	{
		AddRow(TEXT("W A S D"), TEXT("Move"));
		AddRow(TEXT("MOUSE"), TEXT("Look"));
		AddRow(TEXT("LEFT MOUSE"), TEXT("Swing the sword"));
		AddRow(TEXT("SPACE"), TEXT("Jump"));
		AddRow(TEXT("ESC"), TEXT("Pause"));
	}
}

void UDCControlsHintWidget::ShowFor(bool bShotgun, float Seconds)
{
	bShotgunSet = bShotgun;
	Remaining = FMath::Max(0.0f, Seconds);
	BuildRows();
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCControlsHintWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Remaining <= 0.0f)
	{
		return;
	}
	Remaining = FMath::Max(0.0f, Remaining - InDeltaTime);
	if (FadeOutSeconds > 0.0f && Remaining < FadeOutSeconds)
	{
		SetRenderOpacity(Remaining / FadeOutSeconds);
	}
	if (Remaining <= 0.0f)
	{
		RemoveFromParent();
	}
}
