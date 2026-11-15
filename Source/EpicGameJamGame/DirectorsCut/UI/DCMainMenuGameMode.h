// Rated PG for Pig - game mode of the main menu map (Lvl_MainMenu): shows the menu, START opens the gameplay map.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCMainMenuGameMode.generated.h"

class UDCMainMenuWidget;
class USoundBase;
class UAudioComponent;

UCLASS()
class EPICGAMEJAMGAME_API ADCMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ADCMainMenuGameMode();

	/** Map opened by START: the single playable map of the game */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	FName GameplayMap = FName("/Game/DirectorsCut/Maps/Lvl_DirectorsCut");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu")
	TSubclassOf<UDCMainMenuWidget> MenuWidgetClass;

	/** Looping music while the menu is up (fal: Art/Audio/Music_fal/M_Menu_*.wav). Stops when START loads the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu|Music")
	TObjectPtr<USoundBase> MenuMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu|Music", meta=(ClampMin=0.0, ClampMax=2.0))
	float MenuMusicVolume = 0.8f;

	/** Seconds the music fades in at the start and out when START is pressed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu|Music", meta=(ClampMin=0.0, Units="s"))
	float MenuMusicFade = 0.6f;

	UFUNCTION(BlueprintPure, Category="Menu|Music")
	bool IsMenuMusicPlaying() const;

	/** Same as clicking START */
	UFUNCTION(BlueprintCallable, Category="Menu")
	void StartGame();

	UFUNCTION(BlueprintPure, Category="Menu")
	bool IsMenuShown() const { return MenuWidget != nullptr; }

protected:

	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<UDCMainMenuWidget> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MenuMusicComponent;

	bool bStarting = false;
};
