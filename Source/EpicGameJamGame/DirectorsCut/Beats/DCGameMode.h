// Director's Cut - game mode: single-map 5-beat game flow driven by the Director.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCGameMode.generated.h"

class AShooterCharacter;
class AShooterWeapon;
class ADCBoar;
class ADCDirectorBoss;
class ADCKingBoar;
class ADCPrincess;
class UDCHealthComponent;
class UDCHUDWidget;
class ULevelStreaming;

/** List of streaming sublevel names (in /Game/DirectorsCut/Maps/) active during a beat. */
USTRUCT(BlueprintType)
struct FDCBeatSublevels
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats")
	TArray<FName> Sublevels;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCOnBeatChanged, int32, Beat);

/**
 *  Game mode for Lvl_DirectorsCut. Orchestrates the full 5-beat flow:
 *   1. Kill 10 boars with sword (interrupted at 7/10).
 *   2. Collect 5 boar meat (with "this one got no meat" joke).
 *   3. Sword swapped to gun, boar hyperspawn, kill 100 boars.
 *   4. Romance / PG: defeat King Boar and rescue the Princess from the shed.
 *   5. Director enters the scene ("bad actor") -> final boss fight.
 */
UCLASS()
class EPICGAMEJAMGAME_API ADCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ADCGameMode();

	/** Loads the streaming sublevels for Beat (1..5) and unloads all other DC_* sublevels. */
	UFUNCTION(BlueprintCallable, Category="Beats")
	void GoToBeat(int32 Beat);

	/** Full beat transition (sublevels + weapon + spawner + actors + quest + Director VO). */
	UFUNCTION(BlueprintCallable, Category="Beats")
	void StartBeat(int32 Beat);

	/** Starts the Level 1 hunt quest. Safe to call more than once. */
	UFUNCTION(BlueprintCallable, Category="Level 1")
	void StartHuntQuest();

	/** Loads or unloads a single streaming sublevel by name. */
	UFUNCTION(BlueprintCallable, Category="Beats")
	void SetSublevelLoaded(FName SublevelName, bool bLoaded);

	UFUNCTION(BlueprintPure, Category="Beats")
	int32 GetCurrentBeat() const { return CurrentBeat; }

	UFUNCTION(BlueprintPure, Category="UI")
	UDCHUDWidget* GetHUDWidget() const { return HUDWidget; }

	/** Equips either the Beat 1-2 Sword or Beat 3-5 Gun onto the player pawn. */
	UFUNCTION(BlueprintCallable, Category="Weapons")
	void EquipBeatWeapon(bool bUseGun);

	UPROPERTY(BlueprintAssignable, Category="Beats")
	FDCOnBeatChanged OnBeatChanged;

protected:

	/** HUD added to the first local player's screen (WBP_DCHUD) */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UDCHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDCHUDWidget> HUDWidget;

	/** Sublevels loaded for each beat (1..5) */
	UPROPERTY(EditDefaultsOnly, Category="Beats")
	TMap<int32, FDCBeatSublevels> BeatSublevels;

	/** Current active beat (1..5) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Beats")
	int32 CurrentBeat = 1;

	/** Weapon used in Beats 1-2 */
	UPROPERTY(EditDefaultsOnly, Category="Weapons")
	TSubclassOf<AShooterWeapon> SwordWeaponClass;

	/** Weapon used in Beats 3-5 */
	UPROPERTY(EditDefaultsOnly, Category="Weapons")
	TSubclassOf<AShooterWeapon> GunWeaponClass;

	/** Seconds after BeginPlay before the first intro line */
	UPROPERTY(EditDefaultsOnly, Category="Level 1|Intro", meta=(ClampMin=0, Units="s"))
	float IntroDelay = 1.0f;

	/** Director lines played at game start, in order (rows of DT_NarratorLines) */
	UPROPERTY(EditDefaultsOnly, Category="Level 1|Intro")
	TArray<FName> IntroLines;

	UPROPERTY(EditDefaultsOnly, Category="Level 1|Quest")
	FName HuntQuestId = FName("Q_HuntBoars");

	UPROPERTY(EditDefaultsOnly, Category="Level 1|Quest")
	FText HuntQuestTitle;

	UPROPERTY(EditDefaultsOnly, Category="Level 1|Quest", meta=(ClampMin=1))
	int32 HuntQuestTarget = 10;

	UPROPERTY(EditDefaultsOnly, Category="Level 1|Quest")
	FName HuntQuestEventTag = FName("Kill.Boar");

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Level 1")
	void StartIntro();

	void EnterBeat2();
	void EnterBeat3();
	void EnterBeat4();
	void EnterBeat5();
	void CleanupBeatActors();
	void ResetIdleTimer();

	UFUNCTION()
	void HandleLineFinished(FName RowId);

	UFUNCTION()
	void HandleQuestUpdated(FName Id, int32 Current, int32 Target);

	UFUNCTION()
	void HandleQuestCompleted(FName Id);

	UFUNCTION()
	void HandleBoarKilled(ADCBoar* Boar, APawn* Killer);

	UFUNCTION()
	void HandleBoarSpawned(ADCBoar* Boar);

	UFUNCTION()
	void HandleBoarFirstDamaged(UDCHealthComponent* HealthComp, float CurrentHP, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	UFUNCTION()
	void HandlePlayerHealthChanged(float CurrentHP, float MaxHP, float Delta);

	UFUNCTION()
	void HandlePlayerDied(AShooterCharacter* Character);

	UFUNCTION()
	void HandlePrincessLockedAttempt();

	UFUNCTION()
	void HandleBossHalfHP();

	UFUNCTION()
	void HandleBossTaunt();

	void HandleIdleTimeout();
	void StartBossFightNow();

private:

	bool bHuntQuestStarted = false;
	bool bPlayedFirstHit = false;
	bool bPlayedPlayerHurt = false;
	bool bPendingBeat5 = false;
	int32 Beat2BoarKillCount = 0;

	TWeakObjectPtr<ADCPrincess> ActivePrincess;
	TWeakObjectPtr<ADCKingBoar> ActiveKingBoar;
	TWeakObjectPtr<ADCDirectorBoss> ActiveDirectorBoss;
	TWeakObjectPtr<AShooterCharacter> BoundPlayerCharacter;

	FTimerHandle IntroTimer;
	FTimerHandle IdleTimer;
	FTimerHandle BossStartFallbackTimer;
	FTimerHandle Beat5FallbackTimer;
};
