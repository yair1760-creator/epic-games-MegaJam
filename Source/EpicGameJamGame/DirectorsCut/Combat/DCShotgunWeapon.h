// Director's Cut - Beat 3-5 shotgun built on the Shooter weapon class.

#pragma once

#include "CoreMinimal.h"
#include "ShooterWeapon.h"
#include "DCShotgunWeapon.generated.h"

class USoundBase;
class UNiagaraSystem;
class UTexture2D;
class UDCWeaponSpriteWidget;

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

	// ---------------- 2D first-person view (hand-drawn frames, same system as the sword)

	/** Draw the shotgun as 2D hands + gun frames on screen instead of the 3D gun and the template arms */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View")
	bool bUseSpriteView = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View")
	TObjectPtr<UTexture2D> SpriteIdle;

	/** The flash is drawn into this frame; the pose already shows the start of the kick */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View")
	TObjectPtr<UTexture2D> SpriteFlash;

	/** Highest point of the recoil */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View")
	TObjectPtr<UTexture2D> SpriteRecoil;

	/** Gun settling back down */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View")
	TObjectPtr<UTexture2D> SpriteRecovery;

	/** Optional dedicated reload frame; falls back to the drawn recovery/recoil/idle poses if empty */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View")
	TObjectPtr<UTexture2D> SpriteReload;

	/** Animation timing grid (the designer spec: 12 fps, hard cuts, no in-betweens) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=1, ClampMax=60))
	float AnimFPS = 12.0f;

	/** Ticks each pose is held: Muzzle Flash 2 (167 ms) -> Peak Recoil 1 (83 ms) -> Recovery 2 (167 ms) -> Idle */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=1))
	int32 FlashTicks = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=1))
	int32 RecoilTicks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=1))
	int32 RecoveryTicks = 2;

	/** Current pose for UI / tests: Idle, Flash, Recoil, Recovery, ReloadDrop, ReloadLoad, ReloadPump, ReloadSettle */
	UFUNCTION(BlueprintPure, Category="Shotgun|2D View")
	FName GetSpritePose() const { return CurrentPose; }

	/** Seconds since the current shot animation started (-1 when idle) */
	UFUNCTION(BlueprintPure, Category="Shotgun|2D View")
	float GetShotElapsed() const { return ShotTime; }

	/** Seconds since the current reload animation started (-1 when not reloading) */
	UFUNCTION(BlueprintPure, Category="Shotgun|2D View")
	float GetReloadElapsed() const { return ReloadAnimTime; }

	/** True while the front-of-screen 2D reload animation is playing */
	UFUNCTION(BlueprintPure, Category="Shotgun|2D View")
	bool IsPlayingReloadAnim() const { return ReloadAnimTime >= 0.0f; }

	UDCWeaponSpriteWidget* GetSpriteWidget() const { return SpriteWidget; }

	/** Length of one shot animation in seconds ((Flash + Recoil + Recovery) ticks / AnimFPS) */
	UFUNCTION(BlueprintPure, Category="Shotgun|2D View")
	float GetShotAnimDuration() const { return float(FlashTicks + RecoilTicks + RecoveryTicks) / FMath::Max(AnimFPS, 1.0f); }

	/** Frame size on screen (1 = a frame fills the screen height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=0.2, ClampMax=2))
	float SpriteScreenScale = 1.0f;

	/** Walk bob in screen pixels at full running speed (0 = off) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=0))
	float WalkBobPixels = 18.0f;

	/** Gentle breathing sway when standing still, in pixels */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=0))
	float IdleSwayPixels = 4.0f;

	/** Extra pixels the frames are pushed past the bottom-right corner (on top of the bob), so the cut-off
	 *  edges of the drawn arms never show on screen while walking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun|2D View", meta=(ClampMin=0))
	float EdgeBleedPixels = 4.0f;

	virtual void SetActorHiddenInGame(bool bNewHidden) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void StartReload() override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void FireProjectile(const FVector& TargetLocation) override;
	virtual void FinishReload() override;
	virtual void CancelReload() override;

private:

	/** 2D view: create the screen layer for the local player (lazy: the owner may not be possessed yet) */
	void UpdateSpriteView(float DeltaSeconds);
	void RemoveSpriteWidget();
	void SetOwnerArmsHidden(bool bArmsHidden);

	UPROPERTY()
	TObjectPtr<UDCWeaponSpriteWidget> SpriteWidget;

	float ShotTime = -1.0f;         // < 0 when no shot animation is playing
	float ReloadAnimTime = -1.0f;   // < 0 when no reload animation is playing
	float BobTime = 0.0f;
	bool bRecoilShown = false;      // the one-tick peak recoil is never skipped, even on a slow frame
	FName CurrentPose = TEXT("Idle");
};
