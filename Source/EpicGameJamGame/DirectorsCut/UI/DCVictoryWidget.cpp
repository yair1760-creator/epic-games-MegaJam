// Director's Cut - Beat 5 Victory / "THAT'S A WRAP!" end screen.

#include "UI/DCVictoryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UDCVictoryWidget::UDCVictoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

UWidget* UDCVictoryWidget::GetInitialFocusWidget() const
{
	return PlayAgainButton ? static_cast<UWidget*>(PlayAgainButton.Get()) : const_cast<UDCVictoryWidget*>(this);
}

TSharedRef<SWidget> UDCVictoryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
	return Super::RebuildWidget();
}

void UDCVictoryWidget::BuildFallbackLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VictoryRoot"));
	WidgetTree->RootWidget = Root;

	// Fullscreen dark theatrical backdrop
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.01f, 0.03f, 0.86f));
	if (UCanvasPanelSlot* BackdropSlot = Root->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
	}

	// Top and bottom film letterbox bars
	auto AddLetterbox = [this, Root](const TCHAR* Name, float MinY, float MaxY, float AlignY)
	{
		UBorder* Bar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Bar->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
		if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Bar))
		{
			S->SetAnchors(FAnchors(0.0f, MinY, 1.0f, MaxY));
			S->SetAlignment(FVector2D(0.0f, AlignY));
			S->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 68.0f));
		}
	};
	AddLetterbox(TEXT("LetterboxTop"), 0.0f, 0.0f, 0.0f);
	AddLetterbox(TEXT("LetterboxBottom"), 1.0f, 1.0f, 1.0f);

	// Center card
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	Card->SetBrushColor(FLinearColor(0.07f, 0.06f, 0.09f, 0.95f));
	Card->SetPadding(FMargin(56.0f, 42.0f));
	if (UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(Card))
	{
		CardSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Card->SetContent(Column);

	auto MakeText = [this, Column](const TCHAR* Name, const FString& Str, int32 Size, const FLinearColor& Color, float BottomPad) -> UTextBlock*
	{
		UTextBlock* TB = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TB->SetText(FText::FromString(Str));
		FSlateFontInfo Font = TB->GetFont();
		Font.Size = Size;
		TB->SetFont(Font);
		TB->SetColorAndOpacity(FSlateColor(Color));
		TB->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* S = Column->AddChildToVerticalBox(TB))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPad));
		}
		return TB;
	};

	TitleText = MakeText(TEXT("TitleText"), TEXT("CUT! THAT'S A WRAP!"), 52, FLinearColor(1.0f, 0.85f, 0.2f, 1.0f), 10.0f);
	SubtitleText = MakeText(TEXT("SubtitleText"), TEXT("SCENE 1 COMPLETE  -  THE DIRECTOR HAS BEEN RECAST"), 20, FLinearColor(0.95f, 0.95f, 0.95f, 1.0f), 20.0f);
	ReviewText = MakeText(TEXT("ReviewText"), TEXT("\"Terrible performance... five stars.\"  -  The Director  * * * * *"), 22, FLinearColor(1.0f, 0.65f, 0.8f, 1.0f), 22.0f);
	StatsText = MakeText(TEXT("StatsText"), TEXT("Boars Eliminated: 0   |   Run Time: 00:00"), 18, FLinearColor(0.8f, 0.85f, 0.9f, 1.0f), 30.0f);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonsRow"));
	if (UVerticalBoxSlot* BtnRowSlot = Column->AddChildToVerticalBox(Buttons))
	{
		BtnRowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	auto MakeButton = [this, Buttons](const TCHAR* BtnName, const TCHAR* LabelName, const FString& Label, const FLinearColor& BgColor) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), BtnName);
		Btn->SetBackgroundColor(BgColor);
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		Lbl->SetText(FText::FromString(Label));
		FSlateFontInfo Font = Lbl->GetFont();
		Font.Size = 20;
		Lbl->SetFont(Font);
		Lbl->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Btn->SetContent(Lbl);
		if (UHorizontalBoxSlot* S = Buttons->AddChildToHorizontalBox(Btn))
		{
			S->SetPadding(FMargin(14.0f, 0.0f));
		}
		return Btn;
	};

	PlayAgainButton = MakeButton(TEXT("PlayAgainButton"), TEXT("PlayAgainLabel"), TEXT("  PLAY AGAIN  "), FLinearColor(0.18f, 0.58f, 0.25f, 1.0f));
	QuitButton = MakeButton(TEXT("QuitButton"), TEXT("QuitLabel"), TEXT("  QUIT  "), FLinearColor(0.65f, 0.14f, 0.14f, 1.0f));
}

void UDCVictoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayAgainButton)
	{
		PlayAgainButton->OnClicked.AddUniqueDynamic(this, &UDCVictoryWidget::HandlePlayAgainClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UDCVictoryWidget::HandleQuitClicked);
	}
}

void UDCVictoryWidget::NativeDestruct()
{
	if (PlayAgainButton)
	{
		PlayAgainButton->OnClicked.RemoveAll(this);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDCVictoryWidget::SetupVictory(int32 BoarsKilled, float ElapsedSeconds)
{
	CachedBoarsKilled = FMath::Max(0, BoarsKilled);
	CachedElapsedSeconds = FMath::Max(0.0f, ElapsedSeconds);

	if (TitleText)
	{
		TitleText->SetText(NSLOCTEXT("DirectorsCut", "VictoryTitle", "CUT! THAT'S A WRAP!"));
	}
	if (SubtitleText)
	{
		SubtitleText->SetText(NSLOCTEXT("DirectorsCut", "VictorySubtitle", "SCENE 1 COMPLETE  -  THE DIRECTOR HAS BEEN RECAST"));
	}
	if (ReviewText)
	{
		ReviewText->SetText(NSLOCTEXT("DirectorsCut", "VictoryReview", "\"Terrible performance... five stars.\"  -  The Director  * * * * *"));
	}
	if (StatsText)
	{
		const int32 TotalSec = FMath::FloorToInt(CachedElapsedSeconds);
		const int32 Mins = TotalSec / 60;
		const int32 Secs = TotalSec % 60;
		StatsText->SetText(FText::FromString(FString::Printf(
			TEXT("Boars Eliminated: %d   |   Run Time: %02d:%02d   |   Director Defeated"),
			CachedBoarsKilled, Mins, Secs)));
	}

	CurrentOpacity = 0.0f;
	bFadingIn = true;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Visible);
	OnVictoryShown(CachedBoarsKilled, CachedElapsedSeconds);
}

void UDCVictoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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
}

FString UDCVictoryWidget::GetTitleString() const
{
	return TitleText ? TitleText->GetText().ToString() : FString();
}

FString UDCVictoryWidget::GetStatsString() const
{
	return StatsText ? StatsText->GetText().ToString() : FString();
}

void UDCVictoryWidget::HandlePlayAgainClicked()
{
	RestartGame();
}

void UDCVictoryWidget::HandleQuitClicked()
{
	QuitGame();
}

void UDCVictoryWidget::RestartGame()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
		const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		UGameplayStatics::OpenLevel(World, FName(*LevelName));
	}
}

void UDCVictoryWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
