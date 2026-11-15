// Rated PG for Pig - main menu and in-game pause menu.

#include "UI/DCMenuWidgets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Styling/SlateTypes.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------- shared helpers

static void DCPlaceCentred(UWidget* Widget, UCanvasPanel* Canvas, const FVector2D& Offset, const FVector2D& Size, int32 ZOrder)
{
	if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget))
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(Offset);
		Slot->SetSize(Size);
		Slot->SetZOrder(ZOrder);
	}
}

UImage* UDCMenuWidgetBase::AddPicture(UCanvasPanel* Canvas, UTexture2D* Texture, const FVector2D& Offset, const FVector2D& Size, int32 ZOrder)
{
	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (Texture)
	{
		Image->SetBrushFromTexture(Texture, false);
	}
	Image->SetVisibility(ESlateVisibility::HitTestInvisible);
	DCPlaceCentred(Image, Canvas, Offset, Size, ZOrder);
	return Image;
}

UButton* UDCMenuWidgetBase::AddPictureButton(UCanvasPanel* Canvas, UTexture2D* Texture, const FVector2D& Offset, const FVector2D& Size, int32 ZOrder, FName Name)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = Size;
	Brush.DrawAs = ESlateBrushDrawType::Image;

	FButtonStyle Style = Button->GetStyle();
	Style.Normal = Brush;
	Style.Normal.TintColor = FSlateColor(FLinearColor(0.92f, 0.92f, 0.92f, 1.0f));
	Style.Hovered = Brush;
	Style.Hovered.TintColor = FSlateColor(FLinearColor::White);
	Style.Pressed = Brush;
	Style.Pressed.TintColor = FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
	Style.Disabled = Brush;
	Style.Disabled.TintColor = FSlateColor(FLinearColor(0.4f, 0.4f, 0.4f, 0.7f));
	Style.NormalPadding = FMargin(0.0f);
	Style.PressedPadding = FMargin(0.0f, 2.0f, 0.0f, 0.0f);
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
	Button->SetVisibility(ESlateVisibility::Visible);

	DCPlaceCentred(Button, Canvas, Offset, Size, ZOrder);
	return Button;
}

UImage* UDCMenuWidgetBase::AddBackdrop(UCanvasPanel* Canvas, const FLinearColor& Color)
{
	UImage* Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetColorAndOpacity(Color);            // the default brush is a plain white box
	Backdrop->SetVisibility(ESlateVisibility::Visible);   // swallows clicks so nothing behind the menu is hit
	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Backdrop))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetZOrder(0);
	}
	return Backdrop;
}

// ---------------------------------------------------------------- main menu

UDCMainMenuWidget::UDCMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);

	static ConstructorHelpers::FObjectFinder<UTexture2D> Logo(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_Logo_RatedPGForPig.T_UI_Logo_RatedPGForPig"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Start(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_Button_Start.T_UI_Button_Start"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Background(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_MainMenu_BG.T_UI_MainMenu_BG"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Options(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_Button_Options.T_UI_Button_Options"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> NoBudget(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_Options_NoBudget.T_UI_Options_NoBudget"));
	if (Background.Succeeded()) BackgroundTexture = Background.Object;
	if (Logo.Succeeded())  LogoTexture = Logo.Object;
	if (Start.Succeeded()) StartTexture = Start.Object;
	if (Options.Succeeded())  OptionsTexture = Options.Object;
	if (NoBudget.Succeeded()) NoBudgetTexture = NoBudget.Object;
}

TSharedRef<SWidget> UDCMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuCanvas"));
		WidgetTree->RootWidget = Canvas;
		AddBackdrop(Canvas, BackdropColor);

		// the menu picture: fills the screen, same layer as the backdrop but added after it, so it draws on top of
		// the backdrop colour and under the logo (1) and the START button (2). Clicks pass through to the backdrop.
		if (BackgroundTexture)
		{
			UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MenuBackground"));
			Background->SetBrushFromTexture(BackgroundTexture, /*bMatchSize*/ false);
			Background->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UCanvasPanelSlot* BgSlot = Canvas->AddChildToCanvas(Background))
			{
				BgSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				BgSlot->SetOffsets(FMargin(0.0f));
				BgSlot->SetZOrder(0);
			}
		}

		AddPicture(Canvas, LogoTexture, LogoOffset, LogoSize, 1);
		StartButton = AddPictureButton(Canvas, StartTexture, StartOffset, StartSize, 2, TEXT("StartButton"));
		if (OptionsTexture)
		{
			OptionsButton = AddPictureButton(Canvas, OptionsTexture, OptionsOffset, OptionsSize, 2, TEXT("OptionsButton"));
		}

		// "not enough budget" card, hidden until OPTIONS is clicked: a full-screen dim layer that is itself a button
		// (a click anywhere closes the card), and the card picture on top of it (clicks pass through to the dim layer)
		if (NoBudgetTexture)
		{
			NoBudgetDim = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NoBudgetDim"));
			FSlateBrush DimBrush;                                   // no picture: a plain box, tinted to a see-through black
			DimBrush.DrawAs = ESlateBrushDrawType::Box;
			DimBrush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, NoBudgetDimAmount));
			FButtonStyle DimStyle = NoBudgetDim->GetStyle();
			DimStyle.Normal = DimBrush; DimStyle.Hovered = DimBrush; DimStyle.Pressed = DimBrush; DimStyle.Disabled = DimBrush;
			DimStyle.NormalPadding = FMargin(0.0f); DimStyle.PressedPadding = FMargin(0.0f);
			NoBudgetDim->SetStyle(DimStyle);
			NoBudgetDim->SetBackgroundColor(FLinearColor::White);
			if (UCanvasPanelSlot* DimSlot = Canvas->AddChildToCanvas(NoBudgetDim))
			{
				DimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				DimSlot->SetOffsets(FMargin(0.0f));
				DimSlot->SetZOrder(10);
			}
			NoBudgetPanel = AddPicture(Canvas, NoBudgetTexture, FVector2D::ZeroVector, NoBudgetSize, 11);
			NoBudgetDim->SetVisibility(ESlateVisibility::Collapsed);
			NoBudgetPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return Super::RebuildWidget();
}

void UDCMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (StartButton)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UDCMainMenuWidget::HandleStartClicked);
	}
	if (OptionsButton)
	{
		OptionsButton->OnClicked.AddUniqueDynamic(this, &UDCMainMenuWidget::HandleOptionsClicked);
	}
	if (NoBudgetDim)
	{
		NoBudgetDim->OnClicked.AddUniqueDynamic(this, &UDCMainMenuWidget::HandleNoBudgetClosed);
	}
}

UWidget* UDCMainMenuWidget::GetInitialFocusWidget() const
{
	return StartButton ? static_cast<UWidget*>(StartButton.Get()) : const_cast<UDCMainMenuWidget*>(this);
}

FReply UDCMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// while the budget card is up, Esc / Enter / Space (or the gamepad's A or B) only close it
	if (bNoBudgetOpen)
	{
		if (Key == EKeys::Escape || Key == EKeys::Enter || Key == EKeys::SpaceBar
			|| Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Right)
		{
			SetNoBudgetOpen(false);
			return FReply::Handled();
		}
		return FReply::Handled();
	}

	// Enter / Space start the game too (a focused button takes them first: OPTIONS opens the card instead)
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		HandleStartClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDCMainMenuWidget::HandleStartClicked()
{
	if (bNoBudgetOpen)
	{
		return;
	}
	OnStart.Broadcast();
}

void UDCMainMenuWidget::HandleOptionsClicked()
{
	SetNoBudgetOpen(true);
}

void UDCMainMenuWidget::HandleNoBudgetClosed()
{
	SetNoBudgetOpen(false);
}

void UDCMainMenuWidget::SetNoBudgetOpen(bool bOpen)
{
	if (!NoBudgetDim || !NoBudgetPanel)
	{
		return;
	}
	bNoBudgetOpen = bOpen;
	NoBudgetDim->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	NoBudgetPanel->SetVisibility(bOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (StartButton)   StartButton->SetIsEnabled(!bOpen);
	if (OptionsButton) OptionsButton->SetIsEnabled(!bOpen);

	// keyboard focus: on the dim layer while the card is up (Enter / Space click it = close), back on OPTIONS after
	if (UWidget* Focus = bOpen ? static_cast<UWidget*>(NoBudgetDim.Get()) : static_cast<UWidget*>(OptionsButton.Get()))
	{
		Focus->SetKeyboardFocus();
	}
}

// ---------------------------------------------------------------- pause menu

UDCPauseMenuWidget::UDCPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);

	static ConstructorHelpers::FObjectFinder<UTexture2D> Panel(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_Pause_Panel.T_UI_Pause_Panel"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Resume(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_Button_Resume.T_UI_Button_Resume"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Quit(TEXT("/Game/DirectorsCut/UI/Menu/T_UI_Button_Quit.T_UI_Button_Quit"));
	if (Panel.Succeeded())  PanelTexture = Panel.Object;
	if (Resume.Succeeded()) ResumeTexture = Resume.Object;
	if (Quit.Succeeded())   QuitTexture = Quit.Object;
}

TSharedRef<SWidget> UDCPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseCanvas"));
		WidgetTree->RootWidget = Canvas;
		AddBackdrop(Canvas, FLinearColor(0.0f, 0.0f, 0.0f, DimAmount));      // dims the game view
		AddPicture(Canvas, PanelTexture, FVector2D::ZeroVector, PanelSize, 1);
		ResumeButton = AddPictureButton(Canvas, ResumeTexture, ResumeOffset, ButtonSize, 2, TEXT("ResumeButton"));
		QuitButton = AddPictureButton(Canvas, QuitTexture, QuitOffset, ButtonSize, 2, TEXT("QuitButton"));
	}
	return Super::RebuildWidget();
}

void UDCPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UDCPauseMenuWidget::HandleResumeClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UDCPauseMenuWidget::HandleQuitClicked);
	}
}

UWidget* UDCPauseMenuWidget::GetInitialFocusWidget() const
{
	return ResumeButton ? static_cast<UWidget*>(ResumeButton.Get()) : const_cast<UDCPauseMenuWidget*>(this);
}

FReply UDCPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Esc (or P, the stand-in while playing inside the editor) closes the menu again
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::P || Key == EKeys::Gamepad_Special_Right)
	{
		HandleResumeClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDCPauseMenuWidget::HandleResumeClicked()
{
	OnResume.Broadcast();
}

void UDCPauseMenuWidget::HandleQuitClicked()
{
	OnQuit.Broadcast();
}
