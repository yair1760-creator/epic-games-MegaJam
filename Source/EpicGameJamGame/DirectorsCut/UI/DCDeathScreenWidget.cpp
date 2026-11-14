// Director's Cut - "CUT! TAKE 2" Death / Respawn Slate overlay.

#include "UI/DCDeathScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UDCDeathScreenWidget::UDCDeathScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UDCDeathScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
	return Super::RebuildWidget();
}

void UDCDeathScreenWidget::BuildFallbackLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DeathRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.01f, 0.015f, 0.52f));
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Backdrop))
	{
		S->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		S->SetOffsets(FMargin(0.0f));
	}

	auto AddLetterbox = [this, Root](const TCHAR* Name, float MinY, float MaxY, float AlignY)
	{
		UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Bar->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Bar))
		{
			S->SetAnchors(FAnchors(0.0f, MinY, 1.0f, MaxY));
			S->SetAlignment(FVector2D(0.0f, AlignY));
			S->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 56.0f));
		}
	};
	AddLetterbox(TEXT("LetterboxTop"), 0.0f, 0.0f, 0.0f);
	AddLetterbox(TEXT("LetterboxBottom"), 1.0f, 1.0f, 1.0f);

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	Card->SetBrushColor(FLinearColor(0.07f, 0.055f, 0.065f, 0.94f));
	Card->SetPadding(FMargin(48.0f, 32.0f));
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Card))
	{
		S->SetAnchors(FAnchors(0.5f, 0.44f, 0.5f, 0.44f));
		S->SetAlignment(FVector2D(0.5f, 0.44f));
		S->SetAutoSize(true);
	}

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Card->SetContent(Col);

	auto AddText = [this, Col](const TCHAR* Name, const FString& Str, int32 Size, const FLinearColor& Color, float BottomPad) -> UTextBlock*
	{
		UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TB->SetText(FText::FromString(Str));
		FSlateFontInfo Font = TB->GetFont();
		Font.Size = Size;
		TB->SetFont(Font);
		TB->SetColorAndOpacity(FSlateColor(Color));
		TB->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* S = Col->AddChildToVerticalBox(TB))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPad));
		}
		return TB;
	};

	TitleText = AddText(TEXT("TitleText"), TEXT("CUT!  TAKE 2"), 52, FLinearColor(0.95f, 0.78f, 0.22f, 1.0f), 8.0f);
	SubtitleText = AddText(TEXT("SubtitleText"), TEXT("SCENE 1: BOAR HUNT  -  ACTOR DOWN"), 20, FLinearColor(0.96f, 0.93f, 0.86f, 1.0f), 14.0f);
	CountdownText = AddText(TEXT("CountdownText"), TEXT("RESETTING TO MARK IN 5.0s..."), 16, FLinearColor(0.96f, 0.24f, 0.22f, 1.0f), 6.0f);
	AddText(TEXT("ProgressNote"), TEXT("QUEST PROGRESS PRESERVED"), 13, FLinearColor(0.76f, 0.80f, 0.85f, 1.0f), 0.0f);
}

void UDCDeathScreenWidget::SetupDeath(int32 InTakeNumber, const FString& SceneName, float RespawnDuration)
{
	TakeNumber = FMath::Max(2, InTakeNumber);
	RemainingSeconds = FMath::Max(0.0f, RespawnDuration);

	if (TitleText)
	{
		TitleText->SetText(FText::FromString(FString::Printf(TEXT("CUT!  TAKE %d"), TakeNumber)));
	}
	if (SubtitleText)
	{
		const FString DisplayScene = SceneName.IsEmpty() ? TEXT("SCENE 1") : SceneName;
		SubtitleText->SetText(FText::FromString(FString::Printf(TEXT("%s  -  ACTOR DOWN"), *DisplayScene)));
	}
	RefreshCountdownText();

	CurrentOpacity = 0.0f;
	bFadingIn = true;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDCDeathScreenWidget::RefreshCountdownText()
{
	if (!CountdownText)
	{
		return;
	}

	if (RemainingSeconds > 0.05f)
	{
		CountdownText->SetText(FText::FromString(FString::Printf(TEXT("RESETTING TO MARK IN %.1fs..."), RemainingSeconds)));
	}
	else
	{
		CountdownText->SetText(FText::FromString(TEXT("BACK TO YOUR MARK...")));
	}
}

void UDCDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bFadingIn && CurrentOpacity < 1.0f)
	{
		const float Step = FadeInDuration > 0.0f ? InDeltaTime / FadeInDuration : 1.0f;
		CurrentOpacity = FMath::Min(1.0f, CurrentOpacity + Step);
		SetRenderOpacity(CurrentOpacity);
		if (CurrentOpacity >= 1.0f)
		{
			bFadingIn = false;
		}
	}

	if (RemainingSeconds > 0.0f)
	{
		RemainingSeconds = FMath::Max(0.0f, RemainingSeconds - InDeltaTime);
		RefreshCountdownText();
	}
}

FString UDCDeathScreenWidget::GetTitleString() const
{
	return TitleText ? TitleText->GetText().ToString() : FString();
}

FString UDCDeathScreenWidget::GetSubtitleString() const
{
	return SubtitleText ? SubtitleText->GetText().ToString() : FString();
}

FString UDCDeathScreenWidget::GetCountdownString() const
{
	return CountdownText ? CountdownText->GetText().ToString() : FString();
}
