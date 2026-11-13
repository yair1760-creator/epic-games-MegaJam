// Director's Cut - melee sword built on the Shooter weapon class.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "DCSwordWeapon.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UDamageType;
class UTexture2D;
class UDCWeaponSpriteWidget;
class USoundBase;
class UNiagaraSystem;

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
	float Damage = 25.0f;   // 60 HP boar: 60 -> 35 -> 10 (below the 35% flee point) -> dead

	/** How far the swing reaches from the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=10, ClampMax=1000, Units="cm"))
	float Range = 220.0f;

	/** Width of the swing (sweep sphere radius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=1, ClampMax=200, Units="cm"))
	float SwingRadius = 45.0f;

	/** Seconds between swings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword", meta=(ClampMin=0.05, ClampMax=5, Units="s"))
	float Cooldown = 0.5834f;

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

	// ---------------- audio / VFX (empty = silent / nothing; placeholders in /Game/DirectorsCut/Audio/SFX)

	/** Whoosh on every swing (2D, it's the player's own sword) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Audio")
	TObjectPtr<USoundBase> SwingSound;

	/** Played at the target for each actor the blade hits */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|Audio")
	TObjectPtr<USoundBase> HitSound;

	/** Effect at the impact point for each actor hit (NS_HitImpact) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|FX")
	TObjectPtr<UNiagaraSystem> HitFX;

	/** Effect in front of the camera on each swing (NS_SwordTrail) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|FX")
	TObjectPtr<UNiagaraSystem> SwingFX;

	/** Fires on every swing (sounds, Director comments...) */
	UPROPERTY(BlueprintAssignable, Category="Sword")
	FDCOnSwordSwing OnSwing;

	/** Fires for each actor hit */
	UPROPERTY(BlueprintAssignable, Category="Sword")
	FDCOnSwordHit OnHit;

	// ---------------- 2D first-person view (hand-drawn frames, the game's final look)

	/** Draw the weapon as 2D hand + sword frames on screen instead of the 3D sword model */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View")
	bool bUseSpriteView = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View")
	TObjectPtr<UTexture2D> SpriteIdle;

	/** Arm raised, blade back */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View")
	TObjectPtr<UTexture2D> SpriteWindup;

	/** The slash smear across the screen (the hit lands in the middle of this frame) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View")
	TObjectPtr<UTexture2D> SpriteSwing;

	/** Arm extended, blade pointing left */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View")
	TObjectPtr<UTexture2D> SpriteFollow;

	/** Animation timing grid (the designer's spec: 12 fps, hard cuts, no in-betweens) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View", meta=(ClampMin=1, ClampMax=60))
	float AnimFPS = 12.0f;

	/** Ticks each pose is held: Windup 3 (250 ms) -> Swing Smear 1 (83 ms) -> Follow Through 3 (250 ms) -> Idle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View", meta=(ClampMin=1))
	int32 WindupTicks = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View", meta=(ClampMin=1))
	int32 SmearTicks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View", meta=(ClampMin=1))
	int32 FollowTicks = 3;

	/** Current pose for UI / tests: Idle, Windup, Swing, Follow */
	UFUNCTION(BlueprintPure, Category="Sword|2D View")
	FName GetSpritePose() const { return CurrentPose; }

	/** Seconds since the current attack started (-1 when idle) */
	UFUNCTION(BlueprintPure, Category="Sword|2D View")
	float GetAttackElapsed() const { return SwingTime; }

	/** Length of one attack in seconds ((Windup + Smear + Follow) ticks / AnimFPS) */
	UFUNCTION(BlueprintPure, Category="Sword|2D View")
	float GetAttackDuration() const { return float(WindupTicks + SmearTicks + FollowTicks) / FMath::Max(AnimFPS, 1.0f); }

	/** Frame size on screen (1 = a frame fills the screen height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View", meta=(ClampMin=0.2, ClampMax=2))
	float SpriteScreenScale = 1.0f;

	/** Walk bob in screen pixels at full running speed (0 = off) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View", meta=(ClampMin=0))
	float WalkBobPixels = 18.0f;

	/** Gentle breathing sway when standing still, in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sword|2D View", meta=(ClampMin=0))
	float IdleSwayPixels = 4.0f;

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

	/** 2D view: create the screen layer for the local player (lazy: the owner may not be possessed yet) */
	void UpdateSpriteView(float DeltaSeconds);
	void RemoveSpriteWidget();

	/** Seconds from the start of the swing to the hit */
	float GetHitDelay() const;

	UPROPERTY()
	TObjectPtr<UDCWeaponSpriteWidget> SpriteWidget;

	float SwingTime = -1.0f;   // < 0 when not swinging
	float BobTime = 0.0f;
	bool bSmearShown = false;  // the smear tick is never skipped, even on a slow frame
	FName CurrentPose = TEXT("Idle");
	FTimerHandle HitTimer;
};
