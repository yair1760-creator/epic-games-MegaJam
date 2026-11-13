// Director's Cut - shared sound / effect playback.

#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"   // LogDCCombat
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
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
		S.AttenuationShapeExtents = FVector(300.0f, 0.0f, 0.0f);   // full volume inside 3 m
		S.FalloffDistance = 3700.0f;                               // silent at ~40 m
		GWorldAttenuation = Att;
	}
	return GWorldAttenuation.Get();
}

void UDCFXLibrary::PlayWorldSound(const UObject* WorldContext, USoundBase* Sound, FVector Location, float Volume, float PitchJitter, float MinInterval, float PitchScale)
{
	if (!Sound || !WorldContext || !PassesThrottle(Sound, MinInterval))
	{
		return;
	}
	const float Pitch = PitchScale * (1.0f + FMath::FRandRange(-PitchJitter, PitchJitter));
	UE_LOG(LogDCCombat, Verbose, TEXT("SFX 3D %s (vol %.2f, pitch %.2f)"), *Sound->GetName(), Volume, Pitch);
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, FRotator::ZeroRotator, Volume, Pitch, 0.0f, GetWorldAttenuation());
}

void UDCFXLibrary::PlayUISound(const UObject* WorldContext, USoundBase* Sound, float Volume, float PitchJitter, float MinInterval)
{
	if (!Sound || !WorldContext || !PassesThrottle(Sound, MinInterval))
	{
		return;
	}
	const float Pitch = 1.0f + FMath::FRandRange(-PitchJitter, PitchJitter);
	UE_LOG(LogDCCombat, Verbose, TEXT("SFX 2D %s"), *Sound->GetName());
	UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume, Pitch);
}

UNiagaraComponent* UDCFXLibrary::SpawnWorldFX(const UObject* WorldContext, UNiagaraSystem* FX, FVector Location, FRotator Rotation, float Scale)
{
	if (!FX || !WorldContext)
	{
		return nullptr;
	}
	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContext, FX, Location, Rotation, FVector(Scale), true, true);
}
