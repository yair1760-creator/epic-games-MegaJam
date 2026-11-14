// Director's Cut - one place to play gameplay sounds and effects (sword, boars, boss, player, pickups).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "DCFXLibrary.generated.h"

class USoundBase;

/** Constructor-only: default a sound slot to /Game/DirectorsCut/Audio/SFX/<Name> (placeholder until the real file is reimported over it) */
#define DC_DEFAULT_SFX(Prop, Name) 	{ static ConstructorHelpers::FObjectFinder<USoundBase> DCSfxFinder(TEXT("/Game/DirectorsCut/Audio/SFX/" Name "." Name)); 	  if (DCSfxFinder.Succeeded()) { Prop = DCSfxFinder.Object; } }
class USoundAttenuation;
class UNiagaraSystem;
class UNiagaraComponent;
class ADCSpriteFX;

/**
 *  Every sound / effect slot in the combat and AI classes goes through here, so:
 *  - an empty slot is simply silent (the code works before the audio files exist),
 *  - world sounds fade with distance (shared falloff, no attenuation asset needed),
 *  - each play gets a little random pitch so repeats don't sound copy-pasted,
 *  - the same sound can't restart within MinInterval seconds (a 12-boar swarm doesn't stack 12 squeals).
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCFXLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** 3D sound at a location (fades with distance). PitchScale shifts the whole sound (e.g. a deeper King Boar). */
	UFUNCTION(BlueprintCallable, Category="DC|FX", meta=(WorldContext="WorldContext", AdvancedDisplay="PitchJitter,MinInterval,PitchScale"))
	static void PlayWorldSound(const UObject* WorldContext, USoundBase* Sound, FVector Location, float Volume = 1.0f, float PitchJitter = 0.06f, float MinInterval = 0.06f, float PitchScale = 1.0f);

	/** 2D sound (the player's own: hurt, pickup, sword whoosh) */
	UFUNCTION(BlueprintCallable, Category="DC|FX", meta=(WorldContext="WorldContext", AdvancedDisplay="PitchJitter,MinInterval"))
	static void PlayUISound(const UObject* WorldContext, USoundBase* Sound, float Volume = 1.0f, float PitchJitter = 0.04f, float MinInterval = 0.04f);

	/** One-shot Niagara effect at a location (nothing happens if FX is empty) */
	UFUNCTION(BlueprintCallable, Category="DC|FX", meta=(WorldContext="WorldContext"))
	static UNiagaraComponent* SpawnWorldFX(const UObject* WorldContext, UNiagaraSystem* FX, FVector Location, FRotator Rotation = FRotator::ZeroRotator, float Scale = 1.0f);

	/** Hand-drawn one-shot effect (ADCSpriteFX child) at a location, nudged TowardCamera cm toward the player's view
	 *  so it draws in front of the sprite it belongs to (a boar card, a hit target). Empty class = nothing. */
	UFUNCTION(BlueprintCallable, Category="DC|FX", meta=(WorldContext="WorldContext"))
	static ADCSpriteFX* SpawnSpriteFX(const UObject* WorldContext, TSubclassOf<ADCSpriteFX> FXClass, FVector Location, float Scale = 1.0f, float TowardCamera = 30.0f);

	/** Shared distance falloff: full volume within 1.5 m, silent past ~21.5 m */
	static USoundAttenuation* GetWorldAttenuation();

	/** Volume multiplier applied to all SFX while the Director narrator is speaking */
	static constexpr float NarratorSFXDuckScale = 0.22f;

private:

	static bool PassesThrottle(const USoundBase* Sound, float MinInterval);
	static float GetNarratorSFXDuckMultiplier(const UObject* WorldContext);
};
