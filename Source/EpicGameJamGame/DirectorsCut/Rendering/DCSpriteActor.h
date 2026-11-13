// Director's Cut - a standalone camera-facing sprite (props, cardboard cut-outs, the sprite test gallery).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCSpriteActor.generated.h"

class UDCBillboardSprite;

UCLASS(DisplayName="DC Sprite Actor")
class EPICGAMEJAMGAME_API ADCSpriteActor : public AActor
{
	GENERATED_BODY()

public:

	ADCSpriteActor();

	/** Turn in place (degrees per second) so the front / side / back pictures can be checked; 0 = still */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sprite")
	float SpinSpeed = 0.0f;

	UFUNCTION(BlueprintPure, Category="Sprite")
	UDCBillboardSprite* GetSprite() const { return Sprite; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCBillboardSprite> Sprite;

	virtual void Tick(float DeltaSeconds) override;
};
