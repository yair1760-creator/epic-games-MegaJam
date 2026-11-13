// Director's Cut - one place to listen for boar events (quest counter, Director lines, spawner).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCBoarSubsystem.generated.h"

class ADCBoar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCOnBoarKilled, ADCBoar*, Boar, APawn*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCOnBoarSpawned, ADCBoar*, Boar);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCOnMeatCollected, AActor*, Meat, APawn*, Collector);

/**
 *  Tracks living boars in the world and broadcasts when one spawns or dies.
 *  Quest / HUD / Director (other packages): Get World Subsystem (DCBoarSubsystem) -> bind OnBoarKilled.
 *  Killer is the pawn that dealt the final blow (the player for sword kills; may be null).
 */
UCLASS()
class EPICGAMEJAMGAME_API UDCBoarSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category="Boar")
	FDCOnBoarKilled OnBoarKilled;

	UPROPERTY(BlueprintAssignable, Category="Boar")
	FDCOnBoarSpawned OnBoarSpawned;

	/** Boars currently alive */
	UFUNCTION(BlueprintPure, Category="Boar")
	int32 GetAliveCount() const;

	/** Boars killed since the level started */
	UFUNCTION(BlueprintPure, Category="Boar")
	int32 GetKilledCount() const { return KilledCount; }

	UFUNCTION(BlueprintPure, Category="Boar")
	TArray<ADCBoar*> GetAliveBoars() const;

	/** Meat phase of the hunt: when on, every boar that dies drops a meat pickup (off at level start).
	 *  The level flow calls SetMeatDropsEnabled(true) when the Director swaps the quest to "collect meat".
	 *  Console: DC.BoarMeatDrops 1 / 0 */
	UFUNCTION(BlueprintCallable, Category="Boar|Meat")
	void SetMeatDropsEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Boar|Meat")
	bool IsMeatDropsEnabled() const { return bMeatDropsEnabled; }

	/** Fired when the player picks up a piece of meat (the quest itself is told via Collect.Meat) */
	UPROPERTY(BlueprintAssignable, Category="Boar|Meat")
	FDCOnMeatCollected OnMeatCollected;

	/** Called by ADCBoarMeat */
	void NotifyMeatCollected(AActor* Meat, APawn* Collector);

	/** Called by ADCBoar */
	void RegisterBoar(ADCBoar* Boar);
	void NotifyBoarKilled(ADCBoar* Boar, APawn* Killer);

private:

	TArray<TWeakObjectPtr<ADCBoar>> Alive;
	int32 KilledCount = 0;
	bool bMeatDropsEnabled = false;
};
