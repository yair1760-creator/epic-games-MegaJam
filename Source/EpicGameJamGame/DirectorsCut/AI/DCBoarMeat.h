// Director's Cut - boar meat pickup (the "collect meat for bacon" part of the hunt).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCBoarMeat.generated.h"

class USphereComponent;
class UDCBillboardSprite;

/**
 *  Dropped by a dying boar while DCBoarSubsystem meat drops are on.
 *  Pops up and out, lands, then bobs. When the player touches it: reports Collect.Meat to the quest once and disappears.
 */
UCLASS(DisplayName="DC Boar Meat")
class EPICGAMEJAMGAME_API ADCBoarMeat : public AActor
{
	GENERATED_BODY()

public:

	ADCBoarMeat();

	/** Quest event sent on pickup */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meat")
	FName QuestEventTag = TEXT("Collect.Meat");

	/** Seconds after dropping before it can be picked up (lets the pop play) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meat", meta=(ClampMin=0, Units="s"))
	float PickupDelay = 0.4f;

	/** Launch speed of the pop (up and away from the boar) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meat", meta=(ClampMin=0))
	float PopUpSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meat", meta=(ClampMin=0))
	float PopOutSpeed = 160.0f;

	/** Bob height on the ground (cm) and speed (cycles per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meat", meta=(ClampMin=0))
	float BobHeight = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meat", meta=(ClampMin=0))
	float BobSpeed = 0.8f;

	/** Seconds before an uncollected piece disappears (0 = never) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meat", meta=(ClampMin=0, Units="s"))
	float Lifetime = 0.0f;

	/** Sends the pop in this direction (set by the boar before it lands); zero = random */
	void Pop(const FVector& AwayFrom);

	UFUNCTION(BlueprintPure, Category="Meat")
	bool IsCollected() const { return bCollected; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCBillboardSprite> Sprite;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Picks the meat up if Other is the player */
	void TryCollect(AActor* Other);

	/** Height of the level geometry below a point (ignores pawns and pickups) */
	float FindGroundZ(const FVector& At) const;

private:

	bool bCollected = false;
	bool bLanded = false;
	float Age = 0.0f;
	FVector Velocity = FVector::ZeroVector;
	float GroundZ = 0.0f;
};
