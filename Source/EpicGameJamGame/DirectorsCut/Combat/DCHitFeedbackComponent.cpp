// Director's Cut - player hit feedback.

#include "DirectorsCut/Combat/DCHitFeedbackComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

// ---------------------------------------------------------------- shake modifier

void UDCHitShakeModifier::Trigger(float AmplitudeDeg, float Duration)
{
	// keep the stronger of the running and the new shake
	const float RunningNow = TotalTime > 0.0f ? Amplitude * FMath::Square(TimeLeft / TotalTime) : 0.0f;
	if (AmplitudeDeg >= RunningNow)
	{
		Amplitude = AmplitudeDeg;
		TotalTime = FMath::Max(Duration, 0.01f);
		TimeLeft = TotalTime;
	}
}

bool UDCHitShakeModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);

	if (TimeLeft > 0.0f)
	{
		Phase += DeltaTime;
		const float Decay = FMath::Square(TimeLeft / TotalTime);
		const float A = Amplitude * Decay;
		InOutPOV.Rotation.Pitch += A * FMath::Sin(Phase * 47.0f);
		InOutPOV.Rotation.Yaw   += A * 0.7f * FMath::Sin(Phase * 37.0f + 1.3f);
		InOutPOV.Rotation.Roll  += A * 0.5f * FMath::Sin(Phase * 29.0f + 2.1f);
		TimeLeft = FMath::Max(0.0f, TimeLeft - DeltaTime);
	}

	return false;   // let other modifiers run too
}

// ---------------------------------------------------------------- component

UDCHitFeedbackComponent::UDCHitFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDCHitFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	// a respawned player is possessed right after spawning: clear the death fade on the next tick
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UDCHitFeedbackComponent::ClearLeftoverFade);
}

APlayerController* UDCHitFeedbackComponent::GetLocalPC() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return (PC && PC->IsLocalController() && PC->PlayerCameraManager) ? PC : nullptr;
}

void UDCHitFeedbackComponent::ClearLeftoverFade()
{
	if (APlayerController* PC = GetLocalPC())
	{
		PC->PlayerCameraManager->StopCameraFade();
	}
}

void UDCHitFeedbackComponent::PlayHit(float HPFraction)
{
	APlayerController* PC = GetLocalPC();
	if (!PC || HPFraction <= 0.0f)
	{
		return;
	}

	const float Strength = FMath::Clamp(HPFraction / FullStrengthHPFraction, 0.0f, 1.0f);

	// red flash: jump to the alpha, fade back to clear
	const float Alpha = FMath::Lerp(FlashMinAlpha, FlashMaxAlpha, Strength);
	PC->PlayerCameraManager->StartCameraFade(Alpha, 0.0f, FlashDuration, FlashColor, false, false);

	// shake
	UDCHitShakeModifier* Shake = Cast<UDCHitShakeModifier>(PC->PlayerCameraManager->FindCameraModifierByClass(UDCHitShakeModifier::StaticClass()));
	if (!Shake)
	{
		Shake = Cast<UDCHitShakeModifier>(PC->PlayerCameraManager->AddNewCameraModifier(UDCHitShakeModifier::StaticClass()));
	}
	if (Shake)
	{
		Shake->Trigger(FMath::Lerp(ShakeMinDegrees, ShakeMaxDegrees, Strength), ShakeDuration);
	}
}

void UDCHitFeedbackComponent::PlayDeath()
{
	if (APlayerController* PC = GetLocalPC())
	{
		// fade to dark red and hold until the respawned pawn clears it
		PC->PlayerCameraManager->StartCameraFade(0.0f, DeathFadeAlpha, DeathFadeTime, DeathColor, false, true);
	}
}
