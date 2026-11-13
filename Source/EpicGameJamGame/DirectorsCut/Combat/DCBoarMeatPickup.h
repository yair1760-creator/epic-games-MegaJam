// Director's Cut - collectible boar meat / ham dropped during Beat 2.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCBoarMeatPickup.generated.h"

class USphereComponent;
class UDCBillboardSprite;
class UPointLightComponent;
class USoundBase;

UCLASS(DisplayName="DC Boar Meat Pickup")
class EPICGAMEJAMGAME_API ADCBoarMeatPickup : public AActor
{
	GENERATED_BODY()

public:

	ADCBoarMeatPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	FName EventTag = FName("Collect.Meat");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(ClampMin=0))
	float PickupRadius = 170.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(ClampMin=0))
	float MagnetRadius = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup", meta=(ClampMin=0))
	float MagnetSpeed = 520.0f;

	/** 2D blip when the player collects it (empty = silent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup|Audio")
	TObjectPtr<USoundBase> PickupSound;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Camera-facing ham picture (T_DC_BoarMeat_Placeholder until the artist's meat sprite exists) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCBillboardSprite> Sprite;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> GlowLight;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	void Collect();

	float BaseZ = 0.0f;
	float Age = 0.0f;
	bool bCollected = false;
};
