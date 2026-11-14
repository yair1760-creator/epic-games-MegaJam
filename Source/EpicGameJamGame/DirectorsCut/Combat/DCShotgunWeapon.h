// Director's Cut - Beat 3-5 shotgun built on the Shooter weapon class.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "DCShotgunWeapon.generated.h"

class USoundBase;
class UNiagaraSystem;

/**
 *  Pump-action shotgun for Beats 3-5. Fires a cone of multiple pellets per shell,
 *  consumes 1 shell per blast, and gives camera kick + blast audio.
 */
UCLASS(DisplayName="DC Shotgun Weapon")
class EPICGAMEJAMGAME_API ADCShotgunWeapon : public AShooterWeapon
{
	GENERATED_BODY()

public:

	ADCShotgunWeapon();

	/** Number of pellets fired per shotgun blast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun", meta=(ClampMin=1, ClampMax=32))
	int32 PelletCount = 8;

	/** Cone half-angle for pellet spread in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun", meta=(ClampMin=0, ClampMax=45, Units="Degrees"))
	float SpreadAngleDeg = 6.5f;

	/** Scale applied to each spawned pellet projectile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun", meta=(ClampMin=0.1, ClampMax=3.0))
	float PelletScale = 0.6f;

	/** Camera shake amplitude on each shot (degrees, 0 = off) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|Feel", meta=(ClampMin=0, ClampMax=10, Units="Degrees"))
	float FireShakeDegrees = 1.4f;

	/** Sound played when firing a blast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|Audio")
	TObjectPtr<USoundBase> FireSound;

	/** Pitch multiplier for the shotgun blast sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|Audio", meta=(ClampMin=0.25, ClampMax=2.0))
	float FireSoundPitch = 0.85f;

	/** Optional muzzle flash Niagara effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|FX")
	TObjectPtr<UNiagaraSystem> FireFX;

	UFUNCTION(BlueprintPure, Category="Shotgun")
	int32 GetPelletCount() const { return PelletCount; }

	UFUNCTION(BlueprintPure, Category="Shotgun")
	float GetSpreadAngleDeg() const { return SpreadAngleDeg; }

protected:

	virtual void BeginPlay() override;
	virtual void FireProjectile(const FVector& TargetLocation) override;
};
