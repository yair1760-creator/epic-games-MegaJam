// Director's Cut - HUD ammo & reload counter.

#include "UI/DCAmmoCounterWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "ShooterCharacter.h"
#include "ShooterWeapon.h"

void UDCAmmoCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UDCAmmoCounterWidget::HandlePawnChanged);
		HandlePawnChanged(nullptr, PC->GetPawn());
	}
	else
	{
		RefreshDisplay(0, 0, false);
	}
}

void UDCAmmoCounterWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveAll(this);
	}
	Unbind();
	Super::NativeDestruct();
}

void UDCAmmoCounterWidget::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	Unbind();
	if (AShooterCharacter* Character = Cast<AShooterCharacter>(NewPawn))
	{
		Character->OnBulletCountUpdated.AddUniqueDynamic(this, &UDCAmmoCounterWidget::HandleBulletCountUpdated);
		BoundCharacter = Character;
		if (const AShooterWeapon* Weapon = Character->GetCurrentWeapon())
		{
			RefreshDisplay(Weapon->GetMagazineSize(), Weapon->GetBulletCount(), Weapon->IsReloading());
		}
		else
		{
			RefreshDisplay(0, 0, false);
		}
	}
	else
	{
		RefreshDisplay(0, 0, false);
	}
}

void UDCAmmoCounterWidget::Unbind()
{
	if (AShooterCharacter* Character = BoundCharacter.Get())
	{
		Character->OnBulletCountUpdated.RemoveAll(this);
	}
	BoundCharacter.Reset();
}

void UDCAmmoCounterWidget::HandleBulletCountUpdated(int32 MagazineSize, int32 Bullets)
{
	const AShooterCharacter* Character = BoundCharacter.Get();
	const AShooterWeapon* Weapon = Character ? Character->GetCurrentWeapon() : nullptr;
	const bool bReloading = Weapon ? Weapon->IsReloading() : false;
	RefreshDisplay(MagazineSize, Bullets, bReloading);
}

void UDCAmmoCounterWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AShooterCharacter* Character = BoundCharacter.Get();
	const AShooterWeapon* Weapon = Character ? Character->GetCurrentWeapon() : nullptr;
	const int32 MagSize = Weapon ? Weapon->GetMagazineSize() : 0;
	const int32 Bullets = Weapon ? Weapon->GetBulletCount() : 0;
	const bool bReloading = Weapon ? Weapon->IsReloading() : false;

	if (MagSize != CachedMagazineSize || Bullets != CachedBullets || bReloading != bWasReloading)
	{
		RefreshDisplay(MagSize, Bullets, bReloading);
	}
}

void UDCAmmoCounterWidget::RefreshDisplay(int32 MagazineSize, int32 Bullets, bool bReloading)
{
	CachedMagazineSize = FMath::Max(0, MagazineSize);
	CachedBullets = FMath::Max(0, Bullets);
	bWasReloading = bReloading;

	if (CachedMagazineSize <= 0)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (AmmoText)
	{
		AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), CachedBullets, CachedMagazineSize)));
		const FLinearColor AmmoColor = (CachedBullets <= 0 || bReloading)
			? FLinearColor(0.96f, 0.28f, 0.24f, 1.0f)
			: (CachedBullets <= 2 ? FLinearColor(0.96f, 0.72f, 0.22f, 1.0f) : FLinearColor(0.96f, 0.93f, 0.86f, 1.0f));
		AmmoText->SetColorAndOpacity(FSlateColor(AmmoColor));
	}

	if (AmmoBar)
	{
		const float Pct = static_cast<float>(CachedBullets) / static_cast<float>(CachedMagazineSize);
		AmmoBar->SetPercent(FMath::Clamp(Pct, 0.0f, 1.0f));
	}

	if (StatusText)
	{
		if (bReloading)
		{
			StatusText->SetText(FText::FromString(TEXT("RELOADING...")));
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.28f, 0.24f, 1.0f)));
		}
		else if (CachedBullets <= 0)
		{
			StatusText->SetText(FText::FromString(TEXT("EMPTY  |  PRESS R")));
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.28f, 0.24f, 1.0f)));
		}
		else
		{
			StatusText->SetText(FText::FromString(TEXT("PROP SHOTGUN  |  R RELOAD")));
			StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.70f, 0.19f, 1.0f)));
		}
	}
}

FString UDCAmmoCounterWidget::GetDisplayedAmmo() const
{
	return AmmoText ? AmmoText->GetText().ToString() : FString();
}

FString UDCAmmoCounterWidget::GetDisplayedStatus() const
{
	return StatusText ? StatusText->GetText().ToString() : FString();
}
