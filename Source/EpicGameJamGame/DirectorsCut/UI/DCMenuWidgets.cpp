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
	if (Logo.Succeeded())  LogoTexture = Logo.Object;
	if (Start.Succeeded()) StartTexture = Start.Object;
}

TSharedRef<SWidget> UDCMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MenuCanvas"));
		WidgetTree->RootWidget = Canvas;
		AddBackdrop(Canvas, BackdropColor);
		AddPicture(Canvas, LogoTexture, LogoOffset, LogoSize, 1);
		StartButton = AddPictureButton(Canvas, StartTexture, StartOffset, StartSize, 2, TEXT("StartButton"));
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
}

UWidget* UDCMainMenuWidget::GetInitialFocusWidget() const
{
	return StartButton ? static_cast<UWidget*>(StartButton.Get()) : const_cast<UDCMainMenuWidget*>(this);
}

FReply UDCMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Enter / Space start the game too (the button also takes them when it has keyboard focus)
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		HandleStartClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDCMainMenuWidget::HandleStartClicked()
{
	OnStart.Broadcast();
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
