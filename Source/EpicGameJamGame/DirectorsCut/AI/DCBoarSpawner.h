// Director's Cut - keeps a number of boars alive using the level's BoarSpawn points.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCBoarSpawner.generated.h"

class ADCBoar;
class UBillboardComponent;

/**
 *  Drop one in a level. It finds every actor tagged SpawnPointTag ("BoarSpawn" by default: the 6 target points
 *  in the main map and the test map), so no level edits are needed beyond placing the spawner itself.
 *  - Keeps MaxAlive boars alive; a dead boar is replaced after RespawnDelay.
 *  - TotalToSpawn > 0 stops after that many (e.g. a quest that needs exactly 10).
 *  - Skips points the player is standing near (no boars popping in your face) and points already occupied.
 *  - StartSpawning / StopSpawning / DespawnAll for the Director (e.g. the PG beat swaps behaviour).
 */
UCLASS(DisplayName="DC Boar Spawner")
class EPICGAMEJAMGAME_API ADCBoarSpawner : public AActor
{
	GENERATED_BODY()

public:

	ADCBoarSpawner();

	/** Boar to spawn (BP_DC_Boar by default) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	TSubclassOf<ADCBoar> BoarClass;

	/** Actors with this tag are used as spawn points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	FName SpawnPointTag = TEXT("BoarSpawn");

	/** Optional: use exactly these points instead of searching by tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	/** Boars alive at the same time */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin=0, ClampMax=50))
	int32 MaxAlive = 4;

	/** Total boars this spawner will ever spawn (0 = no limit) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin=0))
	int32 TotalToSpawn = 0;

	/** Seconds after a boar dies before a replacement appears */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin=0, Units="s"))
	float RespawnDelay = 4.0f;

	/** Seconds between boars when filling up (start of play, after StartSpawning) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin=0, Units="s"))
	float StaggerDelay = 0.4f;

	/** Don't spawn at a point closer than this to the player */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(ClampMin=0, Units="cm"))
	float MinPlayerDistance = 800.0f;

	/** Start on BeginPlay (off = wait for StartSpawning, e.g. from the Director) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	bool bAutoStart = true;

	/** Area the spawned boars wander in (passed to each boar). Leave empty to use their wander radius only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	TObjectPtr<AActor> WanderArea;

	/** Beat 3 hyperspawn: boars alive at once */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Hyperspawn", meta=(ClampMin=1, ClampMax=50))
	int32 HyperMaxAlive = 12;

	/** Beat 3 hyperspawn: seconds before a dead boar is replaced */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Hyperspawn", meta=(ClampMin=0, Units="s"))
	float HyperRespawnDelay = 0.25f;

	/** Beat 3 hyperspawn: boar HP (one rifle bullet = 25) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Hyperspawn", meta=(ClampMin=1))
	float HyperBoarHP = 25.0f;

	/** Beat 3 hyperspawn: damage per bite (a swarm bites a lot, so lower than a normal boar) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Hyperspawn", meta=(ClampMin=0))
	float HyperBiteDamage = 5.0f;

	UFUNCTION(BlueprintCallable, Category="Spawner")
	void StartSpawning();

	/** Stops making new boars (existing ones stay unless bDespawnExisting) */
	UFUNCTION(BlueprintCallable, Category="Spawner")
	void StopSpawning(bool bDespawnExisting = false);

	/** Removes every living boar from this spawner */
	UFUNCTION(BlueprintCallable, Category="Spawner")
	void DespawnAll();

	/** Switches between normal spawning (Beats 1-2) and aggressive hyperspawn (Beat 3). */
	UFUNCTION(BlueprintCallable, Category="Spawner")
	void SetHyperspawnMode(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Spawner")
	bool IsHyperspawnMode() const { return bHyperspawnMode; }

	UFUNCTION(BlueprintPure, Category="Spawner")
	bool IsSpawning() const { return bSpawning; }

	/** The level flow's on/off switch (Level1_Plan "bActive"): true = StartSpawning, false = StopSpawning (boars stay) */
	UFUNCTION(BlueprintCallable, Category="Spawner")
	void SetActive(bool bNewActive) { bNewActive ? StartSpawning() : StopSpawning(false); }

	UFUNCTION(BlueprintPure, Category="Spawner")
	int32 GetAliveCount() const;

	UFUNCTION(BlueprintPure, Category="Spawner")
	int32 GetSpawnedTotal() const { return SpawnedTotal; }

protected:

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UBillboardComponent> Icon;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleBoarDied(ADCBoar* Boar, APawn* Killer);

private:

	/** Spawns one boar if allowed, then schedules the next fill-up step */
	void FillUp();
	ADCBoar* SpawnOne();
	void GatherSpawnPoints();
	AActor* PickSpawnPoint() const;
	bool CanSpawnMore() const;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> FoundPoints;

	TArray<TWeakObjectPtr<ADCBoar>> Living;
	int32 SpawnedTotal = 0;
	bool bSpawning = false;
	bool bHyperspawnMode = false;
	bool bSavedNormalDefaults = false;
	int32 NormalMaxAlive = 5;
	float NormalRespawnDelay = 4.0f;
	float NormalStaggerDelay = 0.4f;
	float NormalMinPlayerDistance = 800.0f;
	float OverrideBoarMaxHP = 0.0f;
	float OverrideCorpseTime = 0.0f;
	float OverrideBiteDamage = -1.0f;   // < 0 = keep the boar's own
	bool bAutoAggroNewBoars = false;
	FTimerHandle FillTimer;
};
