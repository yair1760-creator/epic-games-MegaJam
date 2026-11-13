// Director's Cut - game mode: single-map game flow driven by the Director.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCGameMode.generated.h"

/**
 *  Game mode for Lvl_DirectorsCut. No team scores, no shooter NPCs.
 *  Level 1: after IntroDelay the Director plays IntroLines, and when the last one ends the hunt quest starts.
 *  Pawn / controller / HUD classes are set in BP_DCGameMode.
 */
UCLASS()
class EPICGAMEJAMGAME_API ADCGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ADCGameMode();

protected:

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

	/** Plays the intro lines (or starts the quest right away if there are none) */
	UFUNCTION(BlueprintCallable, Category="Level 1")
	void StartIntro();

	/** Starts the Level 1 hunt quest. Safe to call more than once. */
	UFUNCTION(BlueprintCallable, Category="Level 1")
	void StartHuntQuest();

	UFUNCTION()
	void HandleLineFinished(FName RowId);

	bool bHuntQuestStarted = false;

	FTimerHandle IntroTimer;
};
