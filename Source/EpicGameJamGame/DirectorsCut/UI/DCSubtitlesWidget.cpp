// Director's Cut - HUD subtitles.

#include "UI/DCSubtitlesWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Narrator/DCNarratorSubsystem.h"

void UDCSubtitlesWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Portrait)
	{
		if (PortraitTexture)
		{
			Portrait->SetBrushFromTexture(PortraitTexture);
			Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Portrait->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	Opacity = 0.0f;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);

	if (UDCNarratorSubsystem* Narrator = GetWorld() ? GetWorld()->GetSubsystem<UDCNarratorSubsystem>() : nullptr)
	{
		Narrator->OnLineStarted.AddUniqueDynamic(this, &UDCSubtitlesWidget::HandleLineStarted);
		Narrator->OnLineFinished.AddUniqueDynamic(this, &UDCSubtitlesWidget::HandleLineFinished);
	}
}

void UDCSubtitlesWidget::NativeDestruct()
{
	if (UDCNarratorSubsystem* Narrator = GetWorld() ? GetWorld()->GetSubsystem<UDCNarratorSubsystem>() : nullptr)
	{
		Narrator->OnLineStarted.RemoveAll(this);
		Narrator->OnLineFinished.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDCSubtitlesWidget::HandleLineStarted(FText Speaker, FText Text, float Duration)
{
	FullText = Text.ToString();
	RevealedChars = 0.0f;
	RevealSpeed = FMath::Max(CharsPerSecond, FullText.Len() / FMath::Max(Duration * TypeTimeFraction, 0.1f));

	SpeakerText->SetText(Speaker);
	LineText->SetText(FText::GetEmpty());

	bShowing = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	OnSubtitleShown(Speaker, Text, Duration);
}

void UDCSubtitlesWidget::HandleLineFinished(FName RowId)
{
	SkipTypewriter();
	bShowing = false;
}

void UDCSubtitlesWidget::SkipTypewriter()
{
	RevealedChars = FullText.Len();
	LineText->SetText(FText::FromString(FullText));
}

FString UDCSubtitlesWidget::GetSpeakerString() const
{
	return SpeakerText ? SpeakerText->GetText().ToString() : FString();
}

void UDCSubtitlesWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bShowing && RevealedChars < FullText.Len())
	{
		const int32 Before = FMath::FloorToInt(RevealedChars);
		RevealedChars = FMath::Min<float>(RevealedChars + RevealSpeed * InDeltaTime, FullText.Len());
		const int32 After = FMath::FloorToInt(RevealedChars);
		if (After != Before)
		{
			LineText->SetText(FText::FromString(FullText.Left(After)));
		}
	}

	const float TargetOpacity = bShowing ? 1.0f : 0.0f;
	if (Opacity != TargetOpacity)
	{
		const float Step = FadeTime > 0.0f ? InDeltaTime / FadeTime : 1.0f;
		Opacity = FMath::Clamp(Opacity + (bShowing ? Step : -Step), 0.0f, 1.0f);
		SetRenderOpacity(Opacity);
		if (Opacity <= 0.0f)
		{
			SetVisibility(ESlateVisibility::Collapsed);
			OnSubtitleHidden();
		}
	}
}
