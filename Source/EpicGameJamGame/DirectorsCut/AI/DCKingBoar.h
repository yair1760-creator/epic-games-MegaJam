// Director's Cut - Beat 4 King Boar guarding the Princess shed.

#pragma once

#include "CoreMinimal.h"
#include "DirectorsCut/AI/DCBoar.h"
#include "DCKingBoar.generated.h"

class UStaticMeshComponent;

UCLASS(DisplayName="DC King Boar")
class EPICGAMEJAMGAME_API ADCKingBoar : public ADCBoar
{
	GENERATED_BODY()

public:

	ADCKingBoar();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> CrownMesh;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
