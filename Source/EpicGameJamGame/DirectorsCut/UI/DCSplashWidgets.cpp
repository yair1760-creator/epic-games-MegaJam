// Rated PG for Pig - Unreal Engine logo splash and content notice (shown once per launch before the main menu).

#include "UI/DCSplashWidgets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"

namespace
{
	FSlateFontInfo DCIntroFont(const TCHAR* Path, int32 Size, const FSlateFontInfo& Fallback)
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

// ---------------------------------------------------------------- shared screen

UDCIntroScreenWidget::UDCIntroScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UDCIntroScreenWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("IntroCanvas"));
		WidgetTree->RootWidget = Canvas;

		// solid black behind the content; it stays fully black while the content fades
		UImage* Black = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Black"));
		Black->SetColorAndOpacity(FLinearColor::Black);          // the default brush is a plain white box
		Black->SetVisibility(ESlateVisibility::Visible);        // takes the clicks, so a click skips
		if (UCanvasPanelSlot* BlackSlot = Canvas->AddChildToCanvas(Black))
		{
			BlackSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BlackSlot->SetOffsets(FMargin(0.0f));
			BlackSlot->SetZOrder(0);
		}

		Content = BuildContent(Canvas);
		if (Content)
		{
			Content->SetVisibility(ESlateVisibility::HitTestInvisible);
			Content->SetRenderOpacity(0.0f);
		}
	}
	return Super::RebuildWidget();
}

void UDCIntroScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bFinished)
	{
		return;
	}
	if (!HasContent())
	{
		Finish();
		return;
	}

	// Warm-up on plain black: the fade starts only once the game draws smoothly (5 frames in a row under 50 ms,
	// or 1.5 s at most), so the first frames after the map load, which arrive late and uneven, never show the logo.
	if (!bWarmedUp)
	{
		WarmupTime += InDeltaTime;
		SmoothFrames = (InDeltaTime < 0.05f) ? SmoothFrames + 1 : 0;
		if (SmoothFrames < 5 && WarmupTime < 1.5f)
		{
			return;
		}
		bWarmedUp = true;
		InDeltaTime = 0.0f;
	}

	Elapsed += FMath::Min(InDeltaTime, 1.0f / 30.0f);   // a slow frame slows the fade a little instead of jumping it

	float Opacity = 1.0f;
	if (Elapsed < FadeInTime)
	{
		Opacity = FadeInTime > 0.0f ? Elapsed / FadeInTime : 1.0f;
	}
	else if (Elapsed > FadeInTime + HoldTime)
	{
		const float Out = Elapsed - FadeInTime - HoldTime;
		Opacity = FadeOutTime > 0.0f ? 1.0f - Out / FadeOutTime : 0.0f;
	}
	if (Content)
	{
		Content->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	}

	if (Elapsed >= FadeInTime + HoldTime + FadeOutTime)
	{
		Finish();
	}
}

FReply UDCIntroScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UE_LOG(LogTemp, Log, TEXT("Intro screen key: %s (repeat %d) at %.2f s"), *InKeyEvent.GetKey().ToString(), InKeyEvent.IsRepeat() ? 1 : 0, Elapsed);
	if (!InKeyEvent.IsRepeat())
	{
		Skip();
	}
	return FReply::Handled();
}

FReply UDCIntroScreenWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UE_LOG(LogTemp, Log, TEXT("Intro screen mouse button at %.2f s"), Elapsed);
	Skip();
	return FReply::Handled();
}

void UDCIntroScreenWidget::Skip()
{
	// the first quarter second ignores input: the press that skipped the screen before this one is still going on
	if (!bFinished && bWarmedUp && Elapsed >= 0.25f)
	{
		Finish();
	}
}

void UDCIntroScreenWidget::Finish()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	UE_LOG(LogTemp, Log, TEXT("Intro screen %s finished after %.2f s (%s)"), *GetClass()->GetName(), Elapsed,
		!HasContent() ? TEXT("nothing to show") : (Elapsed >= FadeInTime + HoldTime + FadeOutTime ? TEXT("played through") : TEXT("skipped")));
	OnFinished.Broadcast();
}

// ---------------------------------------------------------------- "Made with Unreal Engine" splash

UDCSplashWidget::UDCSplashWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FadeInTime = 0.5f; HoldTime = 1.5f; FadeOutTime = 0.5f;
	MadeWithText = NSLOCTEXT("DirectorsCut", "MadeWithUnrealEngine", "Made with Unreal Engine");
	TrademarkNotice = NSLOCTEXT("DirectorsCut", "UETrademarkNotice",
		"Unreal® and Unreal Engine® are trademarks or registered trademarks of Epic Games, Inc. in the United States of America and elsewhere.");
}

UWidget* UDCSplashWidget::BuildContent(UCanvasPanel* Canvas)
{
	// one fading box: the line in the middle of the screen, Epic's trademark notice at the bottom
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SplashContent"));
	if (UCanvasPanelSlot* BoxSlot = Canvas->AddChildToCanvas(Box))
	{
		BoxSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BoxSlot->SetOffsets(FMargin(0.0f));
		BoxSlot->SetZOrder(1);
	}

	const FSlateFontInfo Fallback = UTextBlock::StaticClass()->GetDefaultObject<UTextBlock>()->GetFont();

	UTextBlock* MadeWith = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MadeWithUnrealEngine"));
	MadeWith->SetText(MadeWithText);
	MadeWith->SetFont(DCIntroFont(TEXT("/Game/DirectorsCut/UI/Fonts/Font_Display.Font_Display"), 44, Fallback));
	MadeWith->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	MadeWith->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(MadeWith))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Center);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* Notice = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TrademarkNotice"));
	Notice->SetText(TrademarkNotice);
	Notice->SetFont(DCIntroFont(TEXT("/Game/DirectorsCut/UI/Fonts/Font_Body.Font_Body"), 14, Fallback));
	Notice->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f)));
	Notice->SetJustification(ETextJustify::Center);
	Notice->SetAutoWrapText(true);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Notice))
	{
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(80.0f, 0.0f, 80.0f, 40.0f));
	}
	return Box;
}

// ---------------------------------------------------------------- content notice

UDCContentNoticeWidget::UDCContentNoticeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FadeInTime = 0.5f; HoldTime = 2.5f; FadeOutTime = 0.5f;

	// the only place the notice text lives
	Heading = NSLOCTEXT("DirectorsCut", "NoticeHeading", "CONTENT NOTICE");
	Line1 = NSLOCTEXT("DirectorsCut", "NoticeLine1", "Cartoon slapstick violence against cartoon boars.");
	Line2 = NSLOCTEXT("DirectorsCut", "NoticeLine2", "No real animals were harmed. No blood or gore.");
	CardTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Splash/T_UI_ContentNotice_Card.T_UI_ContentNotice_Card")));
	Quote = NSLOCTEXT("DirectorsCut", "NoticeQuote", "\"Relax. The boars are union actors. They all went home for lunch.\" - The Director");
}

UWidget* UDCContentNoticeWidget::BuildContent(UCanvasPanel* Canvas)
{
	// Drawn card (Art/UI/RatedPG/ContentNotice_Card.png: the same four lines, lettered in the game's style).
	// If the picture is not there, the plain-text version below is shown instead.
	if (UTexture2D* Card = CardTexture.LoadSynchronous())
	{
		const float W = static_cast<float>(Card->GetSizeX());
		const float H = static_cast<float>(FMath::Max(Card->GetSizeY(), 1));
		UImage* CardImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("NoticeCard"));
		CardImage->SetBrushFromTexture(Card, /*bMatchSize*/ false);
		if (UCanvasPanelSlot* CardSlot = Canvas->AddChildToCanvas(CardImage))
		{
			CardSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CardSlot->SetPosition(FVector2D::ZeroVector);
			CardSlot->SetSize(FVector2D(CardWidth, CardWidth * H / W));
			CardSlot->SetZOrder(1);
		}
		return CardImage;
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NoticeBox"));
	if (UCanvasPanelSlot* BoxSlot = Canvas->AddChildToCanvas(Box))
	{
		BoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		BoxSlot->SetPosition(FVector2D::ZeroVector);
		BoxSlot->SetAutoSize(true);
		BoxSlot->SetZOrder(1);
	}

	// the HUD's fonts (Roboto); the italic is the same family, imported for this screen
	const FSlateFontInfo Fallback = UTextBlock::StaticClass()->GetDefaultObject<UTextBlock>()->GetFont();
	const FSlateFontInfo HeadFont = DCIntroFont(TEXT("/Game/DirectorsCut/UI/Fonts/Font_Display.Font_Display"), 56, Fallback);
	const FSlateFontInfo BodyFont = DCIntroFont(TEXT("/Game/DirectorsCut/UI/Fonts/Font_Body.Font_Body"), 28, Fallback);
	const FSlateFontInfo QuoteFont = DCIntroFont(TEXT("/Game/UI/Splash/Font_NoticeItalic.Font_NoticeItalic"), 22, BodyFont);

	const FLinearColor NoticeCream(0.87f, 0.65f, 0.41f, 1.0f);       // the menu's cream (#F0D3AC in sRGB)
	const FLinearColor Soft(0.80f, 0.80f, 0.80f, 1.0f);

	auto AddLine = [this, Box](FName Name, const FText& Text, const FSlateFontInfo& Font, const FLinearColor& Color, float TopPadding)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		T->SetText(Text);
		T->SetFont(Font);
		T->SetColorAndOpacity(FSlateColor(Color));
		T->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(T))
		{
			S->SetHorizontalAlignment(HAlign_Center);
			S->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
		}
	};

	AddLine(TEXT("NoticeHeading"), Heading, HeadFont, NoticeCream, 0.0f);
	AddLine(TEXT("NoticeLine1"), Line1, BodyFont, FLinearColor::White, 36.0f);
	AddLine(TEXT("NoticeLine2"), Line2, BodyFont, FLinearColor::White, 10.0f);
	AddLine(TEXT("NoticeQuote"), Quote, QuoteFont, Soft, 44.0f);
	return Box;
}
