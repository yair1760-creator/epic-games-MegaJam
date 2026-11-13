// Director's Cut - player hit feedback: red screen flash + camera shake (no extra plugins or assets needed).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraModifier.h"
#include "DCHitFeedbackComponent.generated.h"

class APlayerController;

/**
 *  Camera modifier that adds a short, decaying rotation shake to whatever the player is looking through.
 *  Added to the player camera manager on demand by UDCHitFeedbackComponent.
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCHitShakeModifier : public UCameraModifier
{
	GENERATED_BODY()

public:

	/** Starts (or restarts, keeping the stronger one) a shake. Amplitude in degrees. */
	void Trigger(float AmplitudeDeg, float Duration);

	virtual bool ModifyCamera(float DeltaTime, struct FMinimalViewInfo& InOutPOV) override;

private:

	float Amplitude = 0.0f;
	float TotalTime = 0.0f;
	float TimeLeft = 0.0f;
	float Phase = 0.0f;
};

/**
 *  Put on a player pawn. Call PlayHit(Severity) when damaged and PlayDeath() on death.
 *  Only plays when the owner is controlled by a local PlayerController (AI pawns ignore it).
 */
UCLASS(ClassGroup=(DirectorsCut), meta=(BlueprintSpawnableComponent), DisplayName="DC Hit Feedback")
class EPICGAMEJAMGAME_API UDCHitFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UDCHitFeedbackComponent();

	/** Screen flash color on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Flash")
	FLinearColor FlashColor = FLinearColor(0.8f, 0.0f, 0.0f, 1.0f);

	/** Flash opacity for the strongest hit (0..1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Flash", meta=(ClampMin=0, ClampMax=1))
	float FlashMaxAlpha = 0.45f;

	/** Flash opacity for the weakest hit (0..1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Flash", meta=(ClampMin=0, ClampMax=1))
	float FlashMinAlpha = 0.18f;

	/** Seconds for the flash to fade out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Flash", meta=(ClampMin=0.01, Units="s"))
	float FlashDuration = 0.3f;

	/** Shake amplitude (degrees) for the strongest hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Shake", meta=(ClampMin=0, ClampMax=15, Units="Degrees"))
	float ShakeMaxDegrees = 2.5f;

	/** Shake amplitude (degrees) for the weakest hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Shake", meta=(ClampMin=0, ClampMax=15, Units="Degrees"))
	float ShakeMinDegrees = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Shake", meta=(ClampMin=0.01, Units="s"))
	float ShakeDuration = 0.25f;

	/** A hit that removes this fraction of max HP (or more) plays the strongest feedback */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback", meta=(ClampMin=0.01, ClampMax=1))
	float FullStrengthHPFraction = 0.2f;

	/** Screen color the view fades to on death (cleared when the player respawns) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Death")
	FLinearColor DeathColor = FLinearColor(0.25f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Death", meta=(ClampMin=0, ClampMax=1))
	float DeathFadeAlpha = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hit Feedback|Death", meta=(ClampMin=0.01, Units="s"))
	float DeathFadeTime = 1.5f;

	/** HPFraction = damage / max HP of this hit */
	UFUNCTION(BlueprintCallable, Category="Hit Feedback")
	void PlayHit(float HPFraction);

	UFUNCTION(BlueprintCallable, Category="Hit Feedback")
	void PlayDeath();

protected:

	virtual void BeginPlay() override;

private:

	APlayerController* GetLocalPC() const;
	void ClearLeftoverFade();
};
