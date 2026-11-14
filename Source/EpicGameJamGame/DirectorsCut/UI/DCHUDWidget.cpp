// Director's Cut - root HUD widget.

#include "UI/DCHUDWidget.h"
#include "Beats/DCGameMode.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UDCHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ADCGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADCGameMode>() : nullptr)
	{
		GM->OnBeatChanged.AddUniqueDynamic(this, &UDCHUDWidget::HandleBeatChanged);
		RefreshTakeLabel(GM->GetCurrentBeat());
	}
}

void UDCHUDWidget::NativeDestruct()
{
	if (ADCGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADCGameMode>() : nullptr)
	{
		GM->OnBeatChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UDCHUDWidget::HandleBeatChanged(int32 Beat)
{
	RefreshTakeLabel(Beat);
}

void UDCHUDWidget::RefreshTakeLabel(int32 Beat)
{
	if (!TakeText)
	{
		return;
	}

	const ADCGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ADCGameMode>() : nullptr;
	const FString SceneName = GM ? GM->GetBeatDisplayName(Beat) : FString::Printf(TEXT("SCENE %d"), Beat);
	const int32 TakeNum = GM ? GM->GetCurrentTakeNumber() : 1;
	if (TakeNum > 1)
	{
		TakeText->SetText(FText::FromString(FString::Printf(TEXT("REC  |  %s  |  TAKE %d"), *SceneName, TakeNum)));
	}
	else
	{
		TakeText->SetText(FText::FromString(FString::Printf(TEXT("REC  |  %s"), *SceneName)));
	}
}

void UDCHUDWidget::SetCrosshairVisible(bool bVisible)
{
	if (Crosshair)
	{
		Crosshair->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
