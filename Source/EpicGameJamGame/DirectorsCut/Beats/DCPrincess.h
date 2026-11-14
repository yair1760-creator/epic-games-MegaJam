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
};
