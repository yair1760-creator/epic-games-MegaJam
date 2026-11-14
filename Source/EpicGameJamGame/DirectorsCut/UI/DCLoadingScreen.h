// Rated PG for Pig - loading screen: a running boar (8 drawings at 12 fps) with a static LOADING text over it.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DCLoadingScreen.generated.h"

class UTexture2D;
struct FDCLoadingAnimState;

/**
 *  Shows the loading indicator on every map load and hides it when the map is ready.
 *
 *  The project had no loading screen, so this uses the engine's movie player: its widget keeps being drawn on the
 *  loading thread while the game thread is blocked by the map load, which is the only way the boar can keep running
 *  during a real load. (The movie player does not run inside the editor, so Play-In-Editor shows no loading screen;
 *  it shows in a standalone or packaged game.)
 *
 *  Both layers (Loading_Boar_01..08 and Loading_Text) share one 256 x 212 canvas and are drawn in the same box,
 *  so they stay aligned. Only the boar picture changes: hard swaps, each drawing held for 1 / AnimFPS seconds.
 */
UCLASS(Config=Game)
class EPICGAMEJAMGAME_API UDCLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Animation speed of the boar: 12 drawings per second = 83.33 ms each, 0.67 s per loop of 8 */
	UPROPERTY(Config, EditAnywhere, Category="Loading")
	float AnimFPS = 12.0f;

	/** The screen stays up at least this long, so a very fast load still shows more than one full loop */
	UPROPERTY(Config, EditAnywhere, Category="Loading")
	float MinimumDisplaySeconds = 1.0f;

	/** Size of the 256 x 212 canvas on a 1920 x 1080 reference screen (1 = pixel for pixel) */
	UPROPERTY(Config, EditAnywhere, Category="Loading")
	float IconScale = 1.5f;

	/** The art is a black silhouette, so the screen behind it is light (the UI art's parchment colour) */
	UPROPERTY(Config, EditAnywhere, Category="Loading")
	FLinearColor BackdropColor = FLinearColor(0.87f, 0.66f, 0.40f, 1.0f);

	/** What happened during the last finished load (for tests and the log) */
	UFUNCTION(BlueprintPure, Category="Loading")
	float GetLastShownSeconds() const { return LastShownSeconds; }

	UFUNCTION(BlueprintPure, Category="Loading")
	int32 GetLastFrameSwitches() const { return LastFrameSwitches; }

	UFUNCTION(BlueprintPure, Category="Loading")
	int32 GetLastDistinctFrames() const { return LastDistinctFrames; }

protected:

	/** Loading_Boar_01..08 in order, then the static text layer */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> BoarFrames;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> TextTexture;

private:

	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	TSharedPtr<FDCLoadingAnimState> AnimState;
	FString LoadingMapName;
	float LastShownSeconds = 0.0f;
	int32 LastFrameSwitches = 0;
	int32 LastDistinctFrames = 0;
};
