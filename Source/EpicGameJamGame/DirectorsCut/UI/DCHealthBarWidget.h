// Director's Cut - HUD player health bar.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCHealthBarWidget.generated.h"

class AShooterCharacter;
class UProgressBar;
class UTextBlock;

/**
 *  Player HP, bound to AShooterCharacter::OnDamaged of whatever pawn the owning player possesses
 *  (rebinds after every respawn). Layout lives in WBP_HealthBar: it needs HealthBar; HealthText is optional.
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category="Health")
	float GetLifePercent() const { return LifePercent; }

protected:

	UPROPERTY(BlueprintReadOnly, Category="Health", meta=(BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	/** Optional, e.g. "75" */
	UPROPERTY(BlueprintReadOnly, Category="Health", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	/** Called on every change. bDecreased is true when the player took damage. */
	UFUNCTION(BlueprintImplementableEvent, Category="Health")
	void OnHealthChanged(float NewPercent, bool bDecreased);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UFUNCTION()
	void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandleDamaged(float NewPercent);

	void Unbind();
	void Apply(float NewPercent);

	TWeakObjectPtr<AShooterCharacter> BoundCharacter;
	float LifePercent = 1.0f;
};
