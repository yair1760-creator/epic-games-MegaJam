// Director's Cut - HUD player health bar.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DCHealthBarWidget.generated.h"

class AShooterCharacter;
class UProgressBar;
class UTextBlock;
class UImage;
class UTexture2D;

/**
 *  Player HP, bound to AShooterCharacter::OnDamaged of whatever pawn the owning player possesses
 *  (rebinds after every respawn). Layout lives in WBP_HealthBar: it needs HealthBar; HealthText is optional.
 */
UCLASS(Abstract)
class EPICGAMEJAMGAME_API UDCHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDCHealthBarWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category="Health")
	float GetLifePercent() const { return LifePercent; }

	// ---------------- drawn HP bar (Art/UI/HPBar): HP_Bar_Full laid over HP_Bar_Empty and clipped from the right

	/** Show the two drawn pictures instead of the plain progress bar */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Drawn Bar")
	bool bUseDrawnBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Drawn Bar")
	TObjectPtr<UTexture2D> BarEmptyTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Drawn Bar")
	TObjectPtr<UTexture2D> BarFullTexture;

	/** Size of both pictures on screen, in 1920 x 1080 reference pixels (the art is 640 x 112) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Drawn Bar")
	FVector2D BarSize = FVector2D(320.0f, 56.0f);

	/** Left and right edge of the inner track in the art's own pixels: the clip edge moves between these two,
	 *  so the frame is never cut and the fill runs from 0 % to 100 % */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Drawn Bar")
	float TrackStartPx = 27.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Drawn Bar")
	float TrackEndPx = 615.0f;

	/** Keep the plain number over the bar (the drawn bar has no number) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health|Drawn Bar")
	bool bShowNumber = false;

	/** How much of the full picture is showing, as a fraction of its width (for UI / tests) */
	UFUNCTION(BlueprintPure, Category="Health|Drawn Bar")
	float GetFillClipFraction() const { return FillClipFraction; }

	UFUNCTION(BlueprintPure, Category="Health|Drawn Bar")
	bool IsDrawnBarActive() const { return FillImage != nullptr; }

protected:

	/** Plain bar from the layout blueprint; hidden while the drawn bar is active */
	UPROPERTY(BlueprintReadOnly, Category="Health", meta=(BindWidgetOptional))
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

	/** Adds the two pictures next to the plain bar and hides the plain bar (once) */
	void BuildDrawnBar();
	void UpdateDrawnFill();

	UPROPERTY(Transient)
	TObjectPtr<UImage> EmptyImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FillImage;

	float FillClipFraction = 1.0f;

	TWeakObjectPtr<AShooterCharacter> BoundCharacter;
	float LifePercent = 1.0f;
};
