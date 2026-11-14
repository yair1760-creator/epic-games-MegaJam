// Rated PG for Pig - game mode of the main menu map (Lvl_MainMenu): shows the menu, START opens the gameplay map.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DCMainMenuGameMode.generated.h"

class UDCMainMenuWidget;

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

	/** Same as clicking START */
	UFUNCTION(BlueprintCallable, Category="Menu")
	void StartGame();

	UFUNCTION(BlueprintPure, Category="Menu")
	bool IsMenuShown() const { return MenuWidget != nullptr; }

protected:

	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<UDCMainMenuWidget> MenuWidget;

	bool bStarting = false;
};
