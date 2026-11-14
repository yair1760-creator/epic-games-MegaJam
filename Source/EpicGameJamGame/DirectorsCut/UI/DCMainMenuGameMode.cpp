// Rated PG for Pig - main menu game mode.

#include "UI/DCMainMenuGameMode.h"
#include "UI/DCMenuWidgets.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"

ADCMainMenuGameMode::ADCMainMenuGameMode()
{
	// nothing to play as in the menu: no HUD, no weapon, just a still camera
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	HUDClass = nullptr;
	MenuWidgetClass = UDCMainMenuWidget::StaticClass();
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

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	UGameplayStatics::OpenLevel(this, GameplayMap);
}
