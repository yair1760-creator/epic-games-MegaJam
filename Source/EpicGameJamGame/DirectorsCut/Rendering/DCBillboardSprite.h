// Director's Cut - 2.5D sprite that always faces the camera (boars now, the Director and props later).

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DCBillboardSprite.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

/** Which picture the sprite is showing */
UENUM(BlueprintType)
enum class EDCSpriteView : uint8
{
	Front,
	Side,
	Back
};

/** A frame animation (flipbook) made of separate textures */
USTRUCT(BlueprintType)
struct FDCSpriteAnim
{
	GENERATED_BODY()

	/** Frames in order (front-view pictures) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	TArray<TObjectPtr<UTexture2D>> Frames;

	/** Frames per second at play rate 1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(ClampMin=0.1))
	float FPS = 8.0f;

	/** Optional seconds per frame (overrides FPS for that frame, e.g. a long idle + a short blink) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	TArray<float> FrameDurations;

	/** Loop, or stop on the last frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	bool bLoop = true;
};

/**
 *  Upright card that turns (yaw only) to face the player, like the enemies in classic 2.5D shooters.
 *  Animations: fill Animations (name -> frames) and call PlayAnimation / PlayOneShot. The current frame
 *  becomes the front picture (and the fallback for side/back when those are empty).
 *  Picks a picture from where the camera is relative to the owner's facing:
 *  - camera in front of the owner -> FrontTexture, behind -> BackTexture (or Front if none),
 *    to the side -> SideTexture, mirrored automatically so the owner always looks the way it is walking.
 *  - Only one texture set? It is used for every view.
 *  Swap pictures at runtime with SetTextures (animation frames, the Director's costume changes...).
 */
UCLASS(ClassGroup=(DirectorsCut), meta=(BlueprintSpawnableComponent), DisplayName="DC Billboard Sprite")
class EPICGAMEJAMGAME_API UDCBillboardSprite : public USceneComponent
{
	GENERATED_BODY()

public:

	UDCBillboardSprite();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	TObjectPtr<UTexture2D> FrontTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	TObjectPtr<UTexture2D> SideTexture;

	/** Optional; Front is used when empty */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	TObjectPtr<UTexture2D> BackTexture;

	/** True if the side picture shows the owner facing right */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	bool bSideArtFacesRight = true;

	/** Card size for the front view in cm (width, height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	FVector2D FrontSize = FVector2D(110.0f, 110.0f);

	/** Card size for the side view in cm (width, height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	FVector2D SideSize = FVector2D(165.0f, 110.0f);

	/** Bottom edge of the card sits on this component (feet on the ground); off = card centred on it */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	bool bPivotAtBottom = true;

	/** Within this many degrees of the owner's front (or back), the front (back) picture is used */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(ClampMin=0, ClampMax=90, Units="Degrees"))
	float FrontViewAngle = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite", meta=(ClampMin=0))
	float Brightness = 1.0f;

	/** Named frame animations (Idle, Run, Charge, Hit, Fall...) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Animation")
	TMap<FName, FDCSpriteAnim> Animations;

	/** Animation started on BeginPlay (None = still picture) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite|Animation")
	FName StartAnimation;

	/** Plays a looping (or held) animation. Does nothing if it is already playing unless bRestart. */
	UFUNCTION(BlueprintCallable, Category="Sprite|Animation")
	void PlayAnimation(FName Name, float PlayRate = 1.0f, bool bRestart = false);

	/** Plays an animation once (e.g. a hit flinch), then goes back to ThenPlay (None = the one that was playing) */
	UFUNCTION(BlueprintCallable, Category="Sprite|Animation")
	void PlayOneShot(FName Name, FName ThenPlay = NAME_None);

	/** Speed of the current animation (e.g. tied to how fast the owner moves) */
	UFUNCTION(BlueprintCallable, Category="Sprite|Animation")
	void SetPlayRate(float PlayRate);

	UFUNCTION(BlueprintPure, Category="Sprite|Animation")
	FName GetCurrentAnimation() const { return CurrentAnim; }

	UFUNCTION(BlueprintPure, Category="Sprite|Animation")
	bool IsPlayingOneShot() const { return bOneShotActive; }

	UFUNCTION(BlueprintCallable, Category="Sprite")
	void SetTextures(UTexture2D* InFront, UTexture2D* InSide, UTexture2D* InBack);

	UFUNCTION(BlueprintCallable, Category="Sprite")
	void SetTint(FLinearColor InTint, float InBrightness = 1.0f);

	UFUNCTION(BlueprintPure, Category="Sprite")
	EDCSpriteView GetCurrentView() const { return CurrentView; }

	UFUNCTION(BlueprintPure, Category="Sprite")
	bool IsMirrored() const { return bMirrored; }

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void ApplyView(EDCSpriteView View, bool bMirror, bool bForce = false);
	UTexture2D* TextureFor(EDCSpriteView View) const;

	UPROPERTY()
	TObjectPtr<UStaticMesh> QuadMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SpriteMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Quad;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID;

	EDCSpriteView CurrentView = EDCSpriteView::Front;
	bool bMirrored = false;

	// ---- animation state
	void TickAnimation(float DeltaTime);
	void ShowFrame(int32 Index);

	FName CurrentAnim;
	float AnimRate = 1.0f;
	float FrameTime = 0.0f;
	int32 FrameIndex = 0;
	bool bAnimFinished = false;
	bool bOneShotActive = false;
	FName OneShotReturn;
	float OneShotReturnRate = 1.0f;
};
