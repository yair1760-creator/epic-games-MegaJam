// Director's Cut - Title / Start Slate overlay.

#include "UI/DCStartScreenWidget.h"
#include "Beats/DCGameMode.h"
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
#include "Kismet/KismetSystemLibrary.h"

UDCStartScreenWidget::UDCStartScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

UWidget* UDCStartScreenWidget::GetInitialFocusWidget() const
{
	return StartButton ? static_cast<UWidget*>(StartButton.Get()) : const_cast<UDCStartScreenWidget*>(this);
}

TSharedRef<SWidget> UDCStartScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
	return Super::RebuildWidget();
}

void UDCStartScreenWidget::BuildFallbackLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StartRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.015f, 0.02f, 0.72f));
	if (UCanvasPanelSlot* S = Root->AddChildToCanvas(Backdrop))
	{
		S->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		S->SetOffsets(FMargin(0.0f));
	}

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Card"));
	Card->SetBrushColor(FLinearColor(0.07f, 0.06f, 0.08f, 0.96f));
	Card->SetPadding(FMargin(54.0f, 40.0f));
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

	TitleText = AddText(TEXT("TitleText"), TEXT("DIRECTOR'S CUT"), 58, FLinearColor(0.95f, 0.78f, 0.22f, 1.0f), 8.0f);
	SubtitleText = AddText(TEXT("SubtitleText"), TEXT("SCENE 1 / TAKE 1  -  A TOTALLY NORMAL FANTASY QUEST"), 20, FLinearColor(0.96f, 0.93f, 0.86f, 1.0f), 20.0f);
	ControlsText = AddText(TEXT("ControlsText"), TEXT("WASD Move   |   SPACE Jump   |   LMB Attack   |   R Reload   |   ESC / P Pause"), 16, FLinearColor(0.8f, 0.82f, 0.86f, 1.0f), 28.0f);

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
		Font.Size = 22;
		Lbl->SetFont(Font);
		Lbl->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Btn->SetContent(Lbl);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Btn))
		{
			S->SetPadding(FMargin(14.0f, 0.0f));
		}
		return Btn;
	};

	StartButton = AddBtn(TEXT("StartButton"), TEXT("StartLabel"), TEXT("  ACTION! (START)  "), FLinearColor(0.18f, 0.6f, 0.25f, 1.0f));
	QuitButton = AddBtn(TEXT("QuitButton"), TEXT("QuitLabel"), TEXT("  QUIT  "), FLinearColor(0.65f, 0.14f, 0.14f, 1.0f));
}

void UDCStartScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StartButton)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UDCStartScreenWidget::HandleStartClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UDCStartScreenWidget::HandleQuitClicked);
	}
}

void UDCStartScreenWidget::NativeDestruct()
{
	if (StartButton)
	{
		StartButton->OnClicked.RemoveAll(this);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveAll(this);
	}
	Super::NativeDestruct();
}

FReply UDCStartScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		StartGame();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDCStartScreenWidget::HandleStartClicked()
{
	StartGame();
}

void UDCStartScreenWidget::HandleQuitClicked()
{
	QuitGame();
}

void UDCStartScreenWidget::StartGame()
{
	if (UWorld* World = GetWorld())
	{
		if (ADCGameMode* GM = World->GetAuthGameMode<ADCGameMode>())
		{
			GM->StartFromTitle();
		}
	}
}

void UDCStartScreenWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
