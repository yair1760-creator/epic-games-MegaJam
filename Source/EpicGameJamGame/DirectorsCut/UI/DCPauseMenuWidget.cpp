// Director's Cut - In-game Pause menu.

#include "UI/DCPauseMenuWidget.h"
#include "Beats/DCGameMode.h"
#include "Beats/DCPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Quest/DCQuestSubsystem.h"

UDCPauseMenuWidget::UDCPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

UWidget* UDCPauseMenuWidget::GetInitialFocusWidget() const
{
	return ResumeButton ? static_cast<UWidget*>(ResumeButton.Get()) : const_cast<UDCPauseMenuWidget*>(this);
}

TSharedRef<SWidget> UDCPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
	return Super::RebuildWidget();
}

void UDCPauseMenuWidget::BuildFallbackLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.01f, 0.01f, 0.02f, 0.78f));
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Backdrop))
	{
		S->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		S->SetOffsets(FMargin(0.0f));
	}

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	Card->SetBrushColor(FLinearColor(0.07f, 0.06f, 0.08f, 0.96f));
	Card->SetPadding(FMargin(52.0f, 38.0f));
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Card))
	{
		S->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
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

	TitleText = AddText(TEXT("TitleText"), TEXT("CUT!  -  ON HOLD"), 46, FLinearColor(0.95f, 0.78f, 0.22f, 1.0f), 10.0f);
	StatusText = AddText(TEXT("StatusText"), TEXT("SCENE 1"), 18, FLinearColor(0.95f, 0.93f, 0.88f, 1.0f), 18.0f);
	ControlsText = AddText(TEXT("ControlsText"), TEXT("WASD Move   |   SPACE Jump   |   LMB Attack   |   R Reload   |   ESC / P Resume"), 15, FLinearColor(0.76f, 0.8f, 0.84f, 1.0f), 28.0f);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonsRow"));
	if (UVerticalBoxSlot* S = Col->AddChildToVerticalBox(Row))
	{
		S->SetHorizontalAlignment(HAlign_Center);
	}

	auto AddBtn = [this, Row](const TCHAR* BtnName, const TCHAR* LblName, const FString& Label, const FLinearColor& Bg) -> UButton*
	{
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), BtnName);
		Btn->SetBackgroundColor(Bg);
		UTextBlock* Lbl = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LblName);
		Lbl->SetText(FText::FromString(Label));
		FSlateFontInfo Font = Lbl->GetFont();
		Font.Size = 20;
		Lbl->SetFont(Font);
		Lbl->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Btn->SetContent(Lbl);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Btn))
		{
			S->SetPadding(FMargin(10.0f, 0.0f));
		}
		return Btn;
	};

	ResumeButton = AddBtn(TEXT("ResumeButton"), TEXT("ResumeLabel"), TEXT("  RESUME  "), FLinearColor(0.18f, 0.58f, 0.25f, 1.0f));
	RestartButton = AddBtn(TEXT("RestartButton"), TEXT("RestartLabel"), TEXT("  RESTART  "), FLinearColor(0.75f, 0.48f, 0.12f, 1.0f));
	QuitButton = AddBtn(TEXT("QuitButton"), TEXT("QuitLabel"), TEXT("  QUIT  "), FLinearColor(0.65f, 0.14f, 0.14f, 1.0f));
}

void UDCPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UDCPauseMenuWidget::HandleResumeClicked);
	}
	if (RestartButton)
	{
		RestartButton->OnClicked.AddUniqueDynamic(this, &UDCPauseMenuWidget::HandleRestartClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UDCPauseMenuWidget::HandleQuitClicked);
	}

	RefreshStatus();
}

void UDCPauseMenuWidget::NativeDestruct()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveAll(this);
	}
	if (RestartButton)
	{
		RestartButton->OnClicked.RemoveAll(this);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

FReply UDCPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::P)
	{
		ResumeGame();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDCPauseMenuWidget::RefreshStatus()
{
	if (!StatusText)
	{
		return;
	}

	UWorld* World = GetWorld();
	const ADCGameMode* GM = World ? World->GetAuthGameMode<ADCGameMode>() : nullptr;
	const UDCQuestSubsystem* Quest = World ? World->GetSubsystem<UDCQuestSubsystem>() : nullptr;

	FString Status = GM ? GM->GetBeatDisplayName(GM->GetCurrentBeat()) : TEXT("SCENE 1");
	if (Quest && Quest->HasActiveQuest())
	{
		Status += FString::Printf(TEXT("   |   %s: %d/%d"),
			*Quest->GetActiveQuestTitle().ToString(),
			Quest->GetCurrent(),
			Quest->GetTarget());
	}
	StatusText->SetText(FText::FromString(Status));
}

void UDCPauseMenuWidget::HandleResumeClicked()
{
	ResumeGame();
}

void UDCPauseMenuWidget::HandleRestartClicked()
{
	RestartGame();
}

void UDCPauseMenuWidget::HandleQuitClicked()
{
	QuitGame();
}

void UDCPauseMenuWidget::ResumeGame()
{
	if (ADCPlayerController* PC = Cast<ADCPlayerController>(GetOwningPlayer()))
	{
		PC->SetPauseMenuOpen(false);
	}
}

void UDCPauseMenuWidget::RestartGame()
{
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			UGameplayStatics::SetGamePaused(World, false);
			PC->SetIgnoreMoveInput(false);
			PC->SetIgnoreLookInput(false);
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
		const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		UGameplayStatics::OpenLevel(World, FName(*LevelName));
	}
}

void UDCPauseMenuWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
