// Director's Cut - HUD quest tracker.

#include "UI/DCQuestTrackerWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Quest/DCQuestSubsystem.h"

static constexpr float BumpDuration = 0.25f;
static constexpr float BlinkDuration = 1.0f;

void UDCQuestTrackerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UDCQuestSubsystem* Quest = GetWorld() ? GetWorld()->GetSubsystem<UDCQuestSubsystem>() : nullptr)
	{
		Quest->OnQuestUpdated.AddUniqueDynamic(this, &UDCQuestTrackerWidget::HandleQuestUpdated);
		Quest->OnQuestCompleted.AddUniqueDynamic(this, &UDCQuestTrackerWidget::HandleQuestCompleted);
		Quest->OnQuestReplaced.AddUniqueDynamic(this, &UDCQuestTrackerWidget::HandleQuestReplaced);
	}
	Refresh();
}

void UDCQuestTrackerWidget::NativeDestruct()
{
	if (UDCQuestSubsystem* Quest = GetWorld() ? GetWorld()->GetSubsystem<UDCQuestSubsystem>() : nullptr)
	{
		Quest->OnQuestUpdated.RemoveAll(this);
		Quest->OnQuestCompleted.RemoveAll(this);
		Quest->OnQuestReplaced.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDCQuestTrackerWidget::Refresh()
{
	const UDCQuestSubsystem* Quest = GetWorld() ? GetWorld()->GetSubsystem<UDCQuestSubsystem>() : nullptr;
	if (!Quest || !Quest->HasActiveQuest())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	TitleText->SetText(FText::Format(NSLOCTEXT("DirectorsCut", "QuestTitleColon", "{0}:"), Quest->GetActiveQuestTitle()));

	FFormatNamedArguments Args;
	Args.Add(TEXT("Current"), Quest->GetCurrent());
	Args.Add(TEXT("Target"), Quest->GetTarget());
	CountText->SetText(FText::Format(CountFormat, Args));
	CountText->SetColorAndOpacity(FSlateColor(Quest->IsActiveQuestComplete()
		? FLinearColor(0.25f, 1.0f, 0.35f, 1.0f)
		: FLinearColor(1.0f, 0.85f, 0.2f, 1.0f)));
}

void UDCQuestTrackerWidget::HandleQuestUpdated(FName Id, int32 Current, int32 Target)
{
	Refresh();

	if (Current != LastCount && LastCount != INDEX_NONE)
	{
		if (BumpAnim)
		{
			PlayAnimation(BumpAnim);
		}
		else
		{
			BumpTime = BumpDuration;
		}
		OnProgressChanged(Current, Target);
	}
	LastCount = Current;
}

void UDCQuestTrackerWidget::HandleQuestCompleted(FName Id)
{
	if (CompleteAnim)
	{
		PlayAnimation(CompleteAnim);
	}
	OnQuestComplete();
}

void UDCQuestTrackerWidget::HandleQuestReplaced(FName OldId, FName NewId)
{
	// the replacement's first OnQuestUpdated must not count as a bump
	LastCount = INDEX_NONE;

	if (QuestUpdatedAnim)
	{
		PlayAnimation(QuestUpdatedAnim);
	}
	else
	{
		BlinkTime = BlinkDuration;
	}
	OnQuestSwapped();
}

void UDCQuestTrackerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BumpTime > 0.0f)
	{
		BumpTime = FMath::Max(0.0f, BumpTime - InDeltaTime);
		const float Scale = 1.0f + 0.3f * FMath::Sin(PI * (1.0f - BumpTime / BumpDuration));
		CountText->SetRenderScale(FVector2D(Scale));
	}

	if (BlinkTime > 0.0f)
	{
		BlinkTime = FMath::Max(0.0f, BlinkTime - InDeltaTime);
		const bool bOn = BlinkTime <= 0.0f || FMath::Fmod(BlinkTime, 0.25f) > 0.125f;
		SetRenderOpacity(bOn ? 1.0f : 0.25f);
	}
}

FString UDCQuestTrackerWidget::GetDisplayedTitle() const
{
	return TitleText ? TitleText->GetText().ToString() : FString();
}

FString UDCQuestTrackerWidget::GetDisplayedCount() const
{
	return CountText ? CountText->GetText().ToString() : FString();
}
