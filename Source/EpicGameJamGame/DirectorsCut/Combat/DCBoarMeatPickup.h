// Director's Cut - collectible boar meat / ham dropped during Beat 2.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCBoarMeatPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UTextRenderComponent;

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

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MeatMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BoneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> GlowLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> LabelText;

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
