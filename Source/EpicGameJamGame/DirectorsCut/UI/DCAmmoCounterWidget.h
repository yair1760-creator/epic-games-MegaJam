// Director's Cut - HUD ammo & reload counter for ranged weapons (Shotgun in Beat 3).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCAmmoCounterWidget.generated.h"

class AShooterCharacter;
class UProgressBar;
class UTextBlock;

/**
 *  Shows shell count and reload status when the player holds a ranged weapon (MagazineSize > 0).
 *  Automatically collapses when unarmed or holding a melee weapon (e.g. the Sword).
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCAmmoCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category="Ammo")
	int32 GetMagazineSize() const { return CachedMagazineSize; }

	UFUNCTION(BlueprintPure, Category="Ammo")
	int32 GetCurrentBullets() const { return CachedBullets; }

	UFUNCTION(BlueprintPure, Category="Ammo")
	bool IsReloadingDisplayed() const { return bWasReloading; }

	UFUNCTION(BlueprintPure, Category="Ammo")
	FString GetDisplayedAmmo() const;

	UFUNCTION(BlueprintPure, Category="Ammo")
	FString GetDisplayedStatus() const;

protected:

	UPROPERTY(BlueprintReadOnly, Category="Ammo", meta=(BindWidget))
	TObjectPtr<UTextBlock> AmmoText;

	UPROPERTY(BlueprintReadOnly, Category="Ammo", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, Category="Ammo", meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> AmmoBar;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleBulletCountUpdated(int32 MagazineSize, int32 Bullets);

	void Unbind();
	void RefreshDisplay(int32 MagazineSize, int32 Bullets, bool bReloading);

	TWeakObjectPtr<AShooterCharacter> BoundCharacter;
	int32 CachedMagazineSize = 0;
	int32 CachedBullets = 0;
	bool bWasReloading = false;
};
