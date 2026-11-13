// Director's Cut - melee sword built on the Shooter weapon class.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "DCSwordWeapon.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCOnSwordHit, AActor*, HitActor, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnSwordSwing);

/**
 *  Melee weapon for the Shooter character. Uses the normal Fire input:
 *  each press swings once (hold to keep swinging if bHoldToSwing).
 *  - Hits: a sphere sweep from the camera, out to Range, catches up to MaxTargetsPerSwing actors.
 *  - Damage goes through UGameplayStatics::ApplyDamage, so DC Health actors and the Shooter player both react.
 *  - The sword is held in front of the camera (no arm animation needed) and swings procedurally.
 *  - No ammo: magazine size 0, reload does nothing.
 */
UCLASS(DisplayName="DC Sword Weapon")
class EPICGAMEJAMGAME_API ADCSwordWeapon : public AShooterWeapon
{
	GENERATED_BODY()

public:

	ADCSwordWeapon();

	/** Damage per hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=0))
	float Damage = 35.0f;

	/** How far the swing reaches from the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=10, ClampMax=1000, Units="cm"))
	float Range = 220.0f;

	/** Width of the swing (sweep sphere radius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=1, ClampMax=200, Units="cm"))
	float SwingRadius = 45.0f;

	/** Seconds between swings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=0.05, ClampMax=5, Units="s"))
	float Cooldown = 0.55f;

	/** Seconds after the swing starts when the hit lands (lets the animation lead in) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=0, ClampMax=1, Units="s"))
	float HitDelay = 0.08f;

	/** Max actors one swing can hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=1, ClampMax=10))
	int32 MaxTargetsPerSwing = 3;

	/** Push applied to characters that are hit (0 = none) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=0, ClampMax=3000))
	float Knockback = 350.0f;

	/** Holding Fire keeps swinging at the cooldown rate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword")
	bool bHoldToSwing = true;

	/** Damage type sent with each hit (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword")
	TSubclassOf<UDamageType> DamageTypeClass;

	/** Small camera kick for the player when a swing connects (degrees, 0 = off) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Feel", meta=(ClampMin=0, ClampMax=10, Units="Degrees"))
	float HitShakeDegrees = 0.9f;

	/** Seconds the swing animation takes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Feel", meta=(ClampMin=0.05, ClampMax=2, Units="s"))
	float SwingDuration = 0.3f;

	/** Where the hand holds the sword, in camera space (X forward, Y right, Z up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Feel")
	FVector HoldOffset = FVector(70.0f, 26.0f, -40.0f);

	/** Resting angle of the sword in the hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Feel")
	FRotator RestRotation = FRotator(-15.0f, 0.0f, -14.0f);

	/** Angle at the end of the swing (blade cuts down and across) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Feel")
	FRotator SwingEndRotation = FRotator(-35.0f, 10.0f, -115.0f);

	/** Draw the hit sweep in the world (debugging) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Debug")
	bool bDebugDraw = false;

	/** Fires on every swing (sounds, Director comments...) */
	UPROPERTY(BlueprintAssignable, Category="Sword")
	FDCOnSwordSwing OnSwing;

	/** Fires for each actor hit */
	UPROPERTY(BlueprintAssignable, Category="Sword")
	FDCOnSwordHit OnHit;

	virtual void SetActorHiddenInGame(bool bNewHidden) override;

protected:

	/** Hand point in front of the camera; the blade hangs off this and rotates for the swing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> HandPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> SwordMesh;

	/** Same mesh with the ink outline material (inverted hull) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> SwordOutline;

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** One swing (replaces firing a projectile) */
	virtual void Fire() override;

	/** Runs the hit sweep and deals damage */
	void DoHitSweep();

private:

	void AttachToOwnerCamera();
	void SetOwnerArmsHidden(bool bArmsHidden);

	float SwingTime = -1.0f;   // < 0 when not swinging
	FTimerHandle HitTimer;
};
