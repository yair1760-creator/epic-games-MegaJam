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
class UAudioComponent;
class UDCHealthComponent;
class UDCHUDWidget;
class UDCDeathScreenWidget;
class UDCStartScreenWidget;
class UDCVictoryWidget;
class ULevelStreaming;
class USoundBase;

/** List of streaming sublevel names (in /Game/DirectorsCut/Maps/) active during a beat. */
USTRUCT(BlueprintType)
struct FDCBeatSublevels
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats")
	TArray<FName> Sublevels;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCOnBeatChanged, int32, Beat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnVictoryTriggered);

/**
 *  Game mode for Lvl_DirectorsCut. Orchestrates the full 5-beat flow:
 *   1. Kill 10 boars with sword (interrupted at 7/10).
 *   2. Collect 5 bacon (with "this one got no meat" joke).
 *   3. Sword swapped to shotgun, boar hyperspawn, kill 100 boars.
 *   4. Romance / PG: swap back to sword, defeat King Boar and rescue the Princess from the shed.
 *   5. Director enters the scene ("bad actor") -> final boss fight (sword).
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

	/** Human-readable scene title for HUD / Pause menu */
	UFUNCTION(BlueprintPure, Category="Beats")
	FString GetBeatDisplayName(int32 Beat) const;

	UFUNCTION(BlueprintPure, Category="Beats|Audio")
	USoundBase* GetActiveMusicAsset() const { return ActiveMusicAsset; }

	UFUNCTION(BlueprintPure, Category="UI")
	UDCHUDWidget* GetHUDWidget() const { return HUDWidget; }

	UFUNCTION(BlueprintPure, Category="UI")
	UDCStartScreenWidget* GetStartScreenWidget() const { return StartScreenWidget; }

	UFUNCTION(BlueprintPure, Category="UI")
	UDCDeathScreenWidget* GetDeathScreenWidget() const { return DeathScreenWidget; }

	UFUNCTION(BlueprintPure, Category="UI")
	UDCVictoryWidget* GetVictoryWidget() const { return VictoryWidget; }

	UFUNCTION(BlueprintPure, Category="Beats|Start")
	bool IsStartScreenShown() const { return bStartScreenShown; }

	/** Shows the Title / Start slate before Beat 1 starts. */
	UFUNCTION(BlueprintCallable, Category="Beats|Start")
	void ShowStartScreen();

	/** Dismisses the Title / Start slate and begins Beat 1 ("ACTION!"). */
	UFUNCTION(BlueprintCallable, Category="Beats|Start")
	void StartFromTitle();

	UFUNCTION(BlueprintPure, Category="Beats|Death")
	bool IsDeathScreenShown() const { return bDeathScreenShown; }

	UFUNCTION(BlueprintPure, Category="Beats|Death")
	int32 GetCurrentTakeNumber() const { return CurrentTakeNumber; }

	/** Shows the "CUT! TAKE N" slate while the player waits to respawn. */
	UFUNCTION(BlueprintCallable, Category="Beats|Death")
	void ShowDeathScreen(float RespawnDuration = 5.0f);

	/** Hides the Death slate once the actor respawns back on their mark. */
	UFUNCTION(BlueprintCallable, Category="Beats|Death")
	void HideDeathScreen();

	UFUNCTION(BlueprintPure, Category="Beats|Victory")
	bool IsVictoryShown() const { return bVictoryShown; }

	/** Displays the Beat 5 Victory / "THAT'S A WRAP!" end screen. Safe to call more than once. */
	UFUNCTION(BlueprintCallable, Category="Beats|Victory")
	void ShowVictoryScene();

	/** Returns true and writes OutTransform if the active beat overrides player spawn/respawn (e.g. Beat 5 arena). */
	UFUNCTION(BlueprintPure, Category="Beats")
	bool GetBeatRespawnTransform(FTransform& OutTransform) const;

	/** Equips either the Sword (Beats 1-2, 4-5) or the Beat 3 Shotgun onto the player pawn. */
	UFUNCTION(BlueprintCallable, Category="Weapons")
	void EquipBeatWeapon(bool bUseGun);

	UPROPERTY(BlueprintAssignable, Category="Beats")
	FDCOnBeatChanged OnBeatChanged;

	UPROPERTY(BlueprintAssignable, Category="Beats|Victory")
	FDCOnVictoryTriggered OnVictoryTriggered;

protected:

	/** HUD added to the first local player's screen (WBP_DCHUD) */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UDCHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDCHUDWidget> HUDWidget;

	/** Title / Start slate shown on launch before Beat 1 begins (WBP_StartScreen) */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UDCStartScreenWidget> StartScreenWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDCStartScreenWidget> StartScreenWidget;

	/** Show the Start / Title slate on BeginPlay (skipped automatically in automated/unattended runs) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Start")
	bool bShowStartScreen = true;

	/** Death / respawn slate shown when the player dies (WBP_DeathScreen) */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UDCDeathScreenWidget> DeathScreenWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDCDeathScreenWidget> DeathScreenWidget;

	/** Victory / wrap screen shown after defeating The Director (WBP_CutScreen) */
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UDCVictoryWidget> VictoryWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDCVictoryWidget> VictoryWidget;

	/** Sublevels loaded for each beat (1..5) */
	UPROPERTY(EditDefaultsOnly, Category="Beats")
	TMap<int32, FDCBeatSublevels> BeatSublevels;

	/** Current active beat (1..5) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Beats")
	int32 CurrentBeat = 1;

	/** Arena spawn / respawn point used during Beat 5 (Director Boss fight) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Beat 5")
	FVector Beat5ArenaSpawnLocation = FVector(-600.0f, 0.0f, 130.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Beat 5")
	FRotator Beat5ArenaSpawnRotation = FRotator(0.0f, 0.0f, 0.0f);

	/** Safety delay after Q_DefeatDirector completes in case B5_DirectorDefeated is interrupted or missing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Victory", meta=(ClampMin=0.1, Units="s"))
	float VictoryDelayAfterDefeat = 11.0f;

	/** Optional victory sting / cut SFX played when the wrap screen appears */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Victory")
	TObjectPtr<USoundBase> VictorySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Victory")
	TObjectPtr<USoundBase> CutSound;

	/** Optional per-beat background music overrides (defaults to /Game/DirectorsCut/Audio/Music/M_*) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Audio")
	TMap<int32, TObjectPtr<USoundBase>> BeatMusic;

	/** Base volume multiplier for background music */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Audio", meta=(ClampMin=0.0, ClampMax=2.0))
	float MusicVolume = 0.45f;

	/** Volume multiplier applied to music while the Director is speaking (VO ducking) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Audio", meta=(ClampMin=0.0, ClampMax=1.0))
	float MusicDuckScale = 0.35f;

	/** Crossfade duration when switching beat music tracks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Audio", meta=(ClampMin=0.0, Units="s"))
	float MusicFadeDuration = 1.0f;

	/** Fade duration when ducking or restoring music around Director lines */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Beats|Audio", meta=(ClampMin=0.0, Units="s"))
	float MusicDuckFadeDuration = 0.25f;

	/** Weapon used in Beats 1-2 and 4-5 */
	UPROPERTY(EditDefaultsOnly, Category="Weapons")
	TSubclassOf<AShooterWeapon> SwordWeaponClass;

	/** Weapon used in Beat 3 (hyperspawn) */
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
	void HandleLineStarted(FText Speaker, FText Text, float Duration);

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

	USoundBase* ResolveMusicForBeat(int32 Beat) const;
	void UpdateMusicForBeat(int32 Beat);
	void StopBeatMusic(float FadeOutSeconds);
	void RefreshMusicDucking();
	float GetTargetMusicVolume() const;

private:

	bool bHuntQuestStarted = false;
	bool bPlayedFirstHit = false;
	bool bPlayedPlayerHurt = false;
	bool bPendingBeat5 = false;
	bool bStartScreenShown = false;
	bool bDeathScreenShown = false;
	bool bVictoryPending = false;
	bool bVictoryShown = false;
	bool bMusicDucked = false;
	int32 CurrentTakeNumber = 1;
	int32 Beat2BoarKillCount = 0;
	float RunStartTime = 0.0f;

	TWeakObjectPtr<ADCPrincess> ActivePrincess;
	TWeakObjectPtr<ADCKingBoar> ActiveKingBoar;
	TWeakObjectPtr<ADCDirectorBoss> ActiveDirectorBoss;
	TWeakObjectPtr<AShooterCharacter> BoundPlayerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveMusicComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ActiveMusicAsset;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> VictoryMusicComponent;

	FTimerHandle IntroTimer;
	FTimerHandle IdleTimer;
	FTimerHandle BossStartFallbackTimer;
	FTimerHandle Beat5FallbackTimer;
	FTimerHandle VictoryFallbackTimer;
	FTimerHandle MusicUnduckTimer;
};
