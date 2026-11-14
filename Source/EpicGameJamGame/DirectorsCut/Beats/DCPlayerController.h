// Director's Cut - player controller (input mappings + clean respawn without the shooter ammo/health HUD).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DCPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UDCPauseMenuWidget;

/**
 *  Player controller for Lvl_DirectorsCut.
 *  Applies Enhanced Input mapping contexts, tags the possessed pawn as "Player",
 *  and respawns at the active checkpoint or PlayerStart_Village while keeping quest progress.
 */
UCLASS()
class EPICGAMEJAMGAME_API ADCPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	ADCPlayerController();

	// ---------------- pause menu (Esc)

	/** Esc: pause the game and show the pause menu, or close it again */
	UFUNCTION(BlueprintCallable, Category="Director's Cut|Pause")
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category="Director's Cut|Pause")
	void OpenPauseMenu();

	/** RESUME: hide the menu and carry on */
	UFUNCTION(BlueprintCallable, Category="Director's Cut|Pause")
	void ClosePauseMenu();

	/** QUIT: leave the game (same as the victory screen's Quit) */
	UFUNCTION(BlueprintCallable, Category="Director's Cut|Pause")
	void QuitFromPauseMenu();

	UFUNCTION(BlueprintPure, Category="Director's Cut|Pause")
	bool IsPauseMenuOpen() const { return PauseWidget != nullptr; }

protected:

	UPROPERTY(EditAnywhere, Category="Director's Cut|Pause")
	TSubclassOf<UDCPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDCPauseMenuWidget> PauseWidget;

	/** Input mapping contexts always applied for this player */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	/** Input mapping contexts skipped when touch controls are active */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn when touch controls are active */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category="Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Pawn class to respawn on death. Defaults to the GameMode's DefaultPawnClass if unset. */
	UPROPERTY(EditAnywhere, Category="Director's Cut|Respawn")
	TSubclassOf<APawn> RespawnPawnClass;

	/** Tag granted to the possessed pawn */
	UPROPERTY(EditAnywhere, Category="Director's Cut|Player")
	FName PlayerPawnTag = FName("Player");

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedActor);

	bool ShouldUseTouchControls() const;
};
