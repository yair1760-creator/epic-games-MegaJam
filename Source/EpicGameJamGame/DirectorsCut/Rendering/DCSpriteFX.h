// Director's Cut - one-shot hand-drawn effect: a camera-facing card that plays its frames once, then removes itself.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCSpriteFX.generated.h"

class UDCBillboardSprite;
class UTexture2D;

/**
 *  Flipbook effect in the game's 2.5D ink style (no Niagara needed). Spawn it with UDCFXLibrary::SpawnSpriteFX,
 *  or make a Blueprint child and fill Frames. Ready-made: ADCFX_HitSplat, ADCFX_BoarPoof, ADCFX_BossDefeat.
 */
UCLASS(DisplayName="DC Sprite FX")
class EPICGAMEJAMGAME_API ADCSpriteFX : public AActor
{
	GENERATED_BODY()

public:

	ADCSpriteFX();

	/** Played once, in order */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite FX")
	TArray<TObjectPtr<UTexture2D>> Frames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite FX", meta=(ClampMin=1))
	float FPS = 14.0f;

	/** Card size in cm at scale 1 (centered on the spawn point) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite FX")
	FVector2D Size = FVector2D(100.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite FX")
	FLinearColor Tint = FLinearColor::White;

	/** Effects are bright (they read in shade too) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite FX", meta=(ClampMin=0))
	float Brightness = 1.3f;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCBillboardSprite> Sprite;

	virtual void BeginPlay() override;
};

/** Sword hit: comic impact star that bursts into shards (~0.25 s) */
UCLASS(DisplayName="DC FX Hit Splat")
class EPICGAMEJAMGAME_API ADCFX_HitSplat : public ADCSpriteFX
{
	GENERATED_BODY()
public:
	ADCFX_HitSplat();
};

/** Boar death: cartoon dust poof (~0.4 s); scaled with the boar (the King's is 2.5x) */
UCLASS(DisplayName="DC FX Boar Poof")
class EPICGAMEJAMGAME_API ADCFX_BoarPoof : public ADCSpriteFX
{
	GENERATED_BODY()
public:
	ADCFX_BoarPoof();
};

/** Director defeated: a big golden poof (~0.55 s) */
UCLASS(DisplayName="DC FX Boss Defeat")
class EPICGAMEJAMGAME_API ADCFX_BossDefeat : public ADCSpriteFX
{
	GENERATED_BODY()
public:
	ADCFX_BossDefeat();
};
