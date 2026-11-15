// Rated PG for Pig - main menu game mode.

#include "UI/DCMainMenuGameMode.h"
#include "UI/DCMenuWidgets.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ADCMainMenuGameMode::ADCMainMenuGameMode()
{
	// nothing to play as in the menu: no HUD, no weapon, just a still camera
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = nullptr;
	MenuWidgetClass = UDCMainMenuWidget::StaticClass();

	// Sonilo take (picked by Yair, Nov 15). The Stable Audio take is M_Menu. Swap by changing this path: the asset
	// loaded here stays in memory, so it cannot be re-imported over while the editor runs.
	static ConstructorHelpers::FObjectFinder<USoundBase> Music(TEXT("/Game/DirectorsCut/Audio/Music/M_Menu_Alt_Sonilo.M_Menu_Alt_Sonilo"));
	if (Music.Succeeded())
	{
		MenuMusic = Music.Object;
	}
}

bool ADCMainMenuGameMode::IsMenuMusicPlaying() const
{
	return MenuMusicComponent && MenuMusicComponent->IsPlaying();
}

void ADCMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	TSubclassOf<UDCMainMenuWidget> WidgetCls = MenuWidgetClass ? MenuWidgetClass : TSubclassOf<UDCMainMenuWidget>(UDCMainMenuWidget::StaticClass());
	MenuWidget = CreateWidget<UDCMainMenuWidget>(PC, WidgetCls);
	if (!MenuWidget)
	{
		return;
	}
	MenuWidget->AddToViewport(100);

	// menu music: a looping 2D sound owned by this map, so loading the level ends it
	if (MenuMusic && !MenuMusicComponent)
	{
		MenuMusicComponent = UGameplayStatics::SpawnSound2D(this, MenuMusic, MenuMusicVolume, 1.0f, 0.0f, nullptr, /*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ false);
		if (MenuMusicComponent && MenuMusicFade > 0.0f)
		{
			MenuMusicComponent->Stop();
			MenuMusicComponent->FadeIn(MenuMusicFade, MenuMusicVolume);
		}
	}
	MenuWidget->OnStart.AddUniqueDynamic(this, &ADCMainMenuGameMode::StartGame);

	FInputModeUIOnly Mode;
	if (UWidget* Focus = MenuWidget->GetInitialFocusWidget())
	{
		Mode.SetWidgetToFocus(Focus->TakeWidget());
	}
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = true;
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
}

void ADCMainMenuGameMode::StartGame()
{
	if (bStarting)
	{
		return;       // a second click while the map is loading
	}
	bStarting = true;

	if (MenuMusicComponent)
	{
		MenuMusicComponent->FadeOut(MenuMusicFade, 0.0f);      // the level load cuts it anyway; this softens the cut
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	UGameplayStatics::OpenLevel(this, GameplayMap);
}
