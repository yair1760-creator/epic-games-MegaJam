// Director's Cut - training dummy for testing damage (test map zone A).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCTrainingDummy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UDCHealthComponent;

/**
 *  A post with a DC Health component and a floating HP readout.
 *  Flashes a hit pop when damaged, drops when it dies and stands back up after RespawnDelay.
 */
UCLASS(DisplayName="DC Training Dummy")
class EPICGAMEJAMGAME_API ADCTrainingDummy : public AActor
{
	GENERATED_BODY()

public:

	ADCTrainingDummy();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> HPText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCHealthComponent> Health;

	/** Seconds after death before the dummy stands up again with full HP (0 = stays down) */
	UPROPERTY(EditAnywhere, Category="Dummy", meta=(ClampMin=0, Units="s"))
	float RespawnDelay = 3.0f;

	/** Size of the scale pop on hit */
	UPROPERTY(EditAnywhere, Category="Dummy", meta=(ClampMin=0, ClampMax=1))
	float HitPopScale = 0.15f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void OnDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser);

	void StandUp();
	void UpdateText();

private:

	FVector BaseScale = FVector::OneVector;
	FRotator BaseRotation = FRotator::ZeroRotator;
	float PopAlpha = 0.0f;
	FTimerHandle RespawnTimer;
};
