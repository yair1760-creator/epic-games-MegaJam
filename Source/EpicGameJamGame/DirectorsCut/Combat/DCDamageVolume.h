// Director's Cut - box that hurts pawns standing in it (test pad now; hazards later: fire, spikes, the Director's whims).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCDamageVolume.generated.h"

class UBoxComponent;

/**
 *  Deals standard Unreal damage (UGameplayStatics::ApplyDamage) to every pawn inside the box,
 *  once on entry and then every TickInterval seconds. Works on the player (Shooter HP) and on
 *  anything with a DC Health component.
 */
UCLASS(DisplayName="DC Damage Volume")
class EPICGAMEJAMGAME_API ADCDamageVolume : public AActor
{
	GENERATED_BODY()

public:

	ADCDamageVolume();

	/** Damage per hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(ClampMin=0))
	float DamagePerTick = 50.0f;

	/** Seconds between hits while a pawn stays inside */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage", meta=(ClampMin=0.05, Units="s"))
	float TickInterval = 0.5f;

	/** Only hurt pawns controlled by a player (ignore AI) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bPlayersOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	bool bEnabled = true;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> Box;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void DamageOverlapping();
	void HurtActor(AActor* Actor);

private:

	FTimerHandle DamageTimer;
};
