// Director's Cut - player hit feedback.

#include "DirectorsCut/Combat/DCHitFeedbackComponent.h"
#include "DirectorsCut/Combat/DCScreenTintWidget.h"
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
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;   // ticks only while a flash / death tint is showing
}

void UDCHitFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// the respawn destroys the dead pawn: its death tint leaves with it
	if (Overlay)
	{
		Overlay->RemoveFromParent();
		Overlay = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

APlayerController* UDCHitFeedbackComponent::GetLocalPC() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return (PC && PC->IsLocalController() && PC->PlayerCameraManager) ? PC : nullptr;
}

UDCScreenTintWidget* UDCHitFeedbackComponent::GetOverlay(APlayerController* PC)
{
	if (!Overlay && PC)
	{
		Overlay = CreateWidget<UDCScreenTintWidget>(PC, UDCScreenTintWidget::StaticClass());
		if (Overlay)
		{
			Overlay->AddToViewport(OverlayZOrder);
			Overlay->SetTint(FLinearColor::Transparent);
		}
	}
	return Overlay;
}

void UDCHitFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	FlashTime += DeltaTime;
	if (bDying)
	{
		DeathTime += DeltaTime;
	}
	UpdateOverlay();
}

void UDCHitFeedbackComponent::UpdateOverlay()
{
	// flash: jumps to its alpha, then fades linearly to clear; death: ramps up and holds
	const float FlashA = FlashTime < FlashDuration ? FlashStartAlpha * (1.0f - FlashTime / FMath::Max(FlashDuration, 0.01f)) : 0.0f;
	const float DeathA = bDying ? DeathFadeAlpha * FMath::Clamp(DeathTime / FMath::Max(DeathFadeTime, 0.01f), 0.0f, 1.0f) : 0.0f;

	FLinearColor Tint = DeathA >= FlashA ? DeathColor : FlashColor;
	Tint.A = FMath::Max(FlashA, DeathA);
	if (Overlay)
	{
		Overlay->SetTint(Tint);
	}

	// nothing left to animate: stop ticking (the death tint holds without ticking once it is fully in)
	if (FlashA <= 0.0f && (!bDying || DeathTime >= DeathFadeTime))
	{
		SetComponentTickEnabled(false);
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

	// red flash on the overlay (never touches the camera fade): jump to the alpha, fade back to clear
	if (GetOverlay(PC))
	{
		const float Alpha = FMath::Lerp(FlashMinAlpha, FlashMaxAlpha, Strength);
		const float StillShowing = FlashTime < FlashDuration ? FlashStartAlpha * (1.0f - FlashTime / FMath::Max(FlashDuration, 0.01f)) : 0.0f;
		FlashStartAlpha = FMath::Max(Alpha, StillShowing);   // rapid hits don't dim a stronger running flash
		FlashTime = 0.0f;
		SetComponentTickEnabled(true);
		UpdateOverlay();
	}

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
		// tint to dark red and hold; the overlay is removed with this pawn when the player respawns
		if (GetOverlay(PC))
		{
			bDying = true;
			DeathTime = 0.0f;
			SetComponentTickEnabled(true);
			UpdateOverlay();
		}
	}
}
