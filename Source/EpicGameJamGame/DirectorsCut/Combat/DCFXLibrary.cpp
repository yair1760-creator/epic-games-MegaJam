// Director's Cut - shared sound / effect playback.

#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"   // LogDCCombat
#include "DirectorsCut/Narrator/DCNarratorSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "DirectorsCut/Rendering/DCSpriteFX.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "HAL/PlatformTime.h"

namespace
{
	TMap<TWeakObjectPtr<const USoundBase>, double> GLastPlayed;
	TWeakObjectPtr<USoundAttenuation> GWorldAttenuation;
}

bool UDCFXLibrary::PassesThrottle(const USoundBase* Sound, float MinInterval)
{
	const double Now = FPlatformTime::Seconds();
	if (MinInterval > 0.0f)
	{
		if (const double* Last = GLastPlayed.Find(Sound); Last && Now - *Last < MinInterval)
		{
			return false;
		}
	}
	if (GLastPlayed.Num() > 256)
	{
		GLastPlayed.Reset();   // keep it small; stale entries only matter for MinInterval
	}
	GLastPlayed.Add(Sound, Now);
	return true;
}

USoundAttenuation* UDCFXLibrary::GetWorldAttenuation()
{
	if (!GWorldAttenuation.IsValid())
	{
		USoundAttenuation* Att = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("ATT_DC_World_Runtime"));
		Att->AddToRoot();
		FSoundAttenuationSettings& S = Att->Attenuation;
		S.bAttenuate = true;
		S.bSpatialize = true;
		S.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
		S.AttenuationShape = EAttenuationShape::Sphere;
		S.AttenuationShapeExtents = FVector(150.0f, 0.0f, 0.0f);   // full volume inside 1.5 m
		S.FalloffDistance = 2000.0f;                               // silent at ~21.5 m
		GWorldAttenuation = Att;
	}
	return GWorldAttenuation.Get();
}

float UDCFXLibrary::GetNarratorSFXDuckMultiplier(const UObject* WorldContext)
{
	if (const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
	{
		if (const UDCNarratorSubsystem* Narrator = World->GetSubsystem<UDCNarratorSubsystem>(); Narrator && Narrator->IsPlaying())
		{
			return NarratorSFXDuckScale;
		}
	}
	return 1.0f;
}

void UDCFXLibrary::PlayWorldSound(const UObject* WorldContext, USoundBase* Sound, FVector Location, float Volume, float PitchJitter, float MinInterval, float PitchScale)
{
	if (!Sound || !WorldContext || !PassesThrottle(Sound, MinInterval))
	{
		return;
	}
	const float EffectiveVolume = Volume * GetNarratorSFXDuckMultiplier(WorldContext);
	const float Pitch = PitchScale * (1.0f + FMath::FRandRange(-PitchJitter, PitchJitter));
	UE_LOG(LogDCCombat, Verbose, TEXT("SFX 3D %s (vol %.2f, pitch %.2f)"), *Sound->GetName(), EffectiveVolume, Pitch);
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, FRotator::ZeroRotator, EffectiveVolume, Pitch, 0.0f, GetWorldAttenuation());
}

void UDCFXLibrary::PlayUISound(const UObject* WorldContext, USoundBase* Sound, float Volume, float PitchJitter, float MinInterval)
{
	if (!Sound || !WorldContext || !PassesThrottle(Sound, MinInterval))
	{
		return;
	}
	const float EffectiveVolume = Volume * GetNarratorSFXDuckMultiplier(WorldContext);
	const float Pitch = 1.0f + FMath::FRandRange(-PitchJitter, PitchJitter);
	UE_LOG(LogDCCombat, Verbose, TEXT("SFX 2D %s (vol %.2f)"), *Sound->GetName(), EffectiveVolume);
	UGameplayStatics::PlaySound2D(WorldContext, Sound, EffectiveVolume, Pitch);
}

UNiagaraComponent* UDCFXLibrary::SpawnWorldFX(const UObject* WorldContext, UNiagaraSystem* FX, FVector Location, FRotator Rotation, float Scale)
{
	if (!FX || !WorldContext)
	{
		return nullptr;
	}
	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContext, FX, Location, Rotation, FVector(Scale), true, true);
}

ADCSpriteFX* UDCFXLibrary::SpawnSpriteFX(const UObject* WorldContext, TSubclassOf<ADCSpriteFX> FXClass, FVector Location, float Scale, float TowardCamera)
{
	UWorld* World = (FXClass && WorldContext) ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	if (TowardCamera != 0.0f)
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			FVector CamLoc;
			FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			Location += (CamLoc - Location).GetSafeNormal2D() * TowardCamera * Scale;
		}
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ADCSpriteFX>(FXClass, FTransform(FRotator::ZeroRotator, Location, FVector(Scale)), Params);
}
