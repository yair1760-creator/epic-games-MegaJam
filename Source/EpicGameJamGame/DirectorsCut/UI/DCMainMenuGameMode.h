// Rated PG for Pig - game mode of the main menu map (Lvl_MainMenu): shows the menu, START opens the gameplay map.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCMainMenuGameMode.generated.h"

class UDCMainMenuWidget;
class UDCIntroScreenWidget;
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

	/** Unreal Engine logo splash, then the content notice, before the menu the first time it loads in a launch */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Menu|Intro")
	bool bShowIntroScreens = true;

protected:

	virtual void BeginPlay() override;

	/** Creates the menu, starts its music and gives it the mouse and focus (what BeginPlay did before the intro screens) */
	void ShowMenu();

	void ShowIntroScreen(TSubclassOf<UDCIntroScreenWidget> ScreenClass);

	UFUNCTION()
	void HandleIntroScreenFinished();

	UPROPERTY(Transient)
	TObjectPtr<UDCIntroScreenWidget> IntroScreen;

	/** The splash pictures, loaded before the splash starts and kept in memory while it plays */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> PreloadedIntroTextures;

	UPROPERTY(Transient)
	TObjectPtr<UDCMainMenuWidget> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MenuMusicComponent;

	bool bStarting = false;
};
