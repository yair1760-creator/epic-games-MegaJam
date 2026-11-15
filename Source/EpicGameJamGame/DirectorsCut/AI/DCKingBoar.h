// Director's Cut - Beat 4 King Boar guarding the Princess shed.

#pragma once

#include "CoreMinimal.h"
#include "DirectorsCut/AI/DCBoar.h"
#include "DCKingBoar.generated.h"

class UDCBillboardSprite;
class UDCBaconBarWidget;
class UDCHealthComponent;

/**
 *  Crown King Bacon Jr: a boar at 2.5x size with a gold tint, Taisho's crown on its head and his bacon boss bar on screen.
 *  Same AI as a normal boar, but never flees and gets angry when the player comes within GuardRadius.
 */
UCLASS(DisplayName="DC King Boar")
class EPICGAMEJAMGAME_API ADCKingBoar : public ADCBoar
{
	GENERATED_BODY()

public:

	ADCKingBoar();

	/** Multiplies the boar picture (gold) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="King Boar")
	FLinearColor KingTint = FLinearColor(1.15f, 0.92f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="King Boar", meta=(ClampMin=0))
	float KingBrightness = 1.25f;

	/** Crown position on the boar card while alive (x = toward the camera, z = up from the card's bottom, before the 2.5x scale) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="King Boar")
	FVector CrownOffset = FVector(6.0f, 0.0f, 86.0f);

	/** Crown position once the boar is lying down dead */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="King Boar")
	FVector DeadCrownOffset = FVector(6.0f, -16.0f, 44.0f);

	/** Gets angry when the player comes this close */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="King Boar", meta=(ClampMin=0, Units="cm"))
	float GuardRadius = 2200.0f;

	/** Enraged charge speed once King Boar drops below 50% HP */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="King Boar", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float EnragedChargeSpeed = 880.0f;

protected:

	/** Taisho's crown picture (T_DC_KingCrown) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCBillboardSprite> CrownSprite;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBaconBarHealth(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser);

	void RemoveBaconBar();

	/** Crown King Bacon Jr's boss bar on screen (Taisho's bacon health bar) */
	UPROPERTY(Transient)
	TObjectPtr<UDCBaconBarWidget> BaconBar;

private:

	bool bEnraged = false;
};
