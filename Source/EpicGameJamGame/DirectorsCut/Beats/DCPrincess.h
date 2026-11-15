// Director's Cut - Beat 4 Princess actor guarded by the King Boar.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCPrincess.generated.h"

class USphereComponent;
class UDCBillboardSprite;
class UPointLightComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnPrincessLockedAttempt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnPrincessRescued);

UCLASS(DisplayName="DC Princess")
class EPICGAMEJAMGAME_API ADCPrincess : public AActor
{
	GENERATED_BODY()

public:

	ADCPrincess();

	UFUNCTION(BlueprintCallable, Category="Romance")
	void UnlockPrincess();

	UFUNCTION(BlueprintPure, Category="Romance")
	bool IsLocked() const { return bLocked; }

	UFUNCTION(BlueprintPure, Category="Romance")
	bool IsRescued() const { return bRescued; }

	UPROPERTY(BlueprintAssignable, Category="Romance")
	FDCOnPrincessLockedAttempt OnLockedAttempt;

	UPROPERTY(BlueprintAssignable, Category="Romance")
	FDCOnPrincessRescued OnPrincessRescued;

	/** The shed glows from outside; when the player is this close to the princess (inside the shed, the doorway is
	 *  about 330 cm away) the lights around her fade down so she can be seen */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Romance|Light", meta=(ClampMin=0, Units="cm"))
	float InsideRadius = 300.0f;

	/** Light strength inside the shed, as a share of the outside glow (was 0.12: still too bright, Yair Nov 15) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Romance|Light", meta=(ClampMin=0, ClampMax=1))
	float InsideLightScale = 0.05f;

	/** Seconds the glow takes to fade down (going in) or back up (going out) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Romance|Light", meta=(ClampMin=0.05, Units="s"))
	float LightFadeTime = 1.2f;

	/** Other lights within this distance of the princess (the shed's own lamp) fade with her light */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Romance|Light", meta=(ClampMin=0, Units="cm"))
	float ShedLightSearchRadius = 400.0f;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> RescueTrigger;

	/** The janitor princess (designer drawing T_DC_Princess_Janitor), always facing the player */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCBillboardSprite> PrincessSprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> RomanceLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> PrincessLabel;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	bool bLocked = true;
	bool bRescued = false;
	float LastLockedPromptTime = -100.0f;
	float BobTime = 0.0f;

	void GatherShedLights();
	void UpdateShedLights(float DeltaSeconds);

	TArray<TWeakObjectPtr<class ULightComponent>> ShedLights;
	TArray<float> ShedLightBaseIntensity;
	float LightAlpha = 1.0f;
	bool bShedLightsGathered = false;
};
