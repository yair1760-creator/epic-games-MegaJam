// Director's Cut - HUD player health bar.

#include "UI/DCHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "ShooterCharacter.h"

void UDCHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UDCHealthBarWidget::HandlePawnChanged);
		HandlePawnChanged(nullptr, PC->GetPawn());
	}
	else
	{
		Apply(1.0f);
	}
}

void UDCHealthBarWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveAll(this);
	}
	Unbind();
	Super::NativeDestruct();
}

void UDCHealthBarWidget::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	Unbind();
	if (AShooterCharacter* Character = Cast<AShooterCharacter>(NewPawn))
	{
		Character->OnDamaged.AddUniqueDynamic(this, &UDCHealthBarWidget::HandleDamaged);
		BoundCharacter = Character;
		Apply(Character->GetHPPercent());
	}
}

void UDCHealthBarWidget::Unbind()
{
	if (AShooterCharacter* Character = BoundCharacter.Get())
	{
		Character->OnDamaged.RemoveAll(this);
	}
	BoundCharacter.Reset();
}

void UDCHealthBarWidget::HandleDamaged(float NewPercent)
{
	Apply(NewPercent);
}

void UDCHealthBarWidget::Apply(float NewPercent)
{
	NewPercent = FMath::Clamp(NewPercent, 0.0f, 1.0f);
	const bool bDecreased = NewPercent < LifePercent;
	LifePercent = NewPercent;

	HealthBar->SetPercent(LifePercent);
	if (HealthText)
	{
		HealthText->SetText(FText::AsNumber(FMath::RoundToInt(LifePercent * 100.0f)));
	}
	OnHealthChanged(LifePercent, bDecreased);
}
