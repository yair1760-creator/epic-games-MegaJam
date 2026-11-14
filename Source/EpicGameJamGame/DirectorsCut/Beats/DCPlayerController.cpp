// Director's Cut - player controller.

#include "Beats/DCPlayerController.h"
#include "Beats/DCGameMode.h"
#include "Blueprint/UserWidget.h"
#include "CheckpointSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "EpicGameJamGame.h"
#include "EpicGameJamGameCameraManager.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "UI/DCMenuWidgets.h"
#include "ShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Input/SVirtualJoystick.h"

ADCPlayerController::ADCPlayerController()
{
	PlayerCameraManagerClass = AEpicGameJamGameCameraManager::StaticClass();

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultIMC(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (DefaultIMC.Succeeded())
	{
		DefaultMappingContexts.Add(DefaultIMC.Object);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> WeaponsIMC(TEXT("/Game/Variant_Shooter/Input/IMC_Weapons.IMC_Weapons"));
	if (WeaponsIMC.Succeeded())
	{
		DefaultMappingContexts.Add(WeaponsIMC.Object);
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLookIMC(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	if (MouseLookIMC.Succeeded())
	{
		MobileExcludedMappingContexts.Add(MouseLookIMC.Object);
	}
}

void ADCPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// arriving from the main menu (UI-only input, visible cursor): hand the mouse back to the game
	if (IsLocalPlayerController())
	{
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}

	if (IsLocalPlayerController() && ShouldUseTouchControls() && MobileControlsWidgetClass)
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
	}
}

void ADCPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// pause menu: Esc. Inside the editor Esc ends the Play session, so P does the same job there.
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ADCPlayerController::TogglePauseMenu).bExecuteWhenPaused = true;
		InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ADCPlayerController::TogglePauseMenu).bExecuteWhenPaused = true;
		InputComponent->BindKey(EKeys::Gamepad_Special_Right, IE_Pressed, this, &ADCPlayerController::TogglePauseMenu).bExecuteWhenPaused = true;
	}

	if (IsLocalPlayerController())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				if (CurrentContext)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}

			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					if (CurrentContext)
					{
						Subsystem->AddMappingContext(CurrentContext, 0);
					}
				}
			}
		}
	}
}

void ADCPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn)
	{
		return;
	}

	InPawn->OnDestroyed.AddUniqueDynamic(this, &ADCPlayerController::OnPawnDestroyed);

	if (!PlayerPawnTag.IsNone())
	{
		InPawn->Tags.AddUnique(PlayerPawnTag);
	}

	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn))
	{
		ShooterCharacter->SetTeam(0);
	}
}

void ADCPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}

	AGameModeBase* GM = World->GetAuthGameMode();
	TSubclassOf<APawn> SpawnClass = RespawnPawnClass ? RespawnPawnClass : (GM ? GM->DefaultPawnClass : TSubclassOf<APawn>());
	if (!SpawnClass)
	{
		UE_LOG(LogEpicGameJamGame, Warning, TEXT("ADCPlayerController: cannot respawn, no RespawnPawnClass or DefaultPawnClass"));
		return;
	}

	FTransform SpawnTransform;
	bool bHaveSpawn = false;

	if (ADCGameMode* DCGM = Cast<ADCGameMode>(GM))
	{
		bHaveSpawn = DCGM->GetBeatRespawnTransform(SpawnTransform);
	}

	if (!bHaveSpawn)
	{
		if (UCheckpointSubsystem* Checkpoints = World->GetSubsystem<UCheckpointSubsystem>())
		{
			if (Checkpoints->HasActiveCheckpoint())
			{
				SpawnTransform = Checkpoints->GetRespawnTransform();
				bHaveSpawn = true;
			}
		}
	}

	if (!bHaveSpawn && GM)
	{
		if (AActor* Start = GM->FindPlayerStart(this))
		{
			SpawnTransform = Start->GetActorTransform();
			bHaveSpawn = true;
		}
	}

	if (!bHaveSpawn)
	{
		if (AActor* FallbackStart = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()))
		{
			SpawnTransform = FallbackStart->GetActorTransform();
			bHaveSpawn = true;
		}
	}

	if (!bHaveSpawn)
	{
		UE_LOG(LogEpicGameJamGame, Warning, TEXT("ADCPlayerController: no PlayerStart found for respawn"));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (APawn* Respawned = World->SpawnActor<APawn>(SpawnClass, SpawnTransform, Params))
	{
		Possess(Respawned);
		SetControlRotation(SpawnTransform.Rotator());
	}
}

bool ADCPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

// ---------------------------------------------------------------- pause menu

void ADCPlayerController::TogglePauseMenu()
{
	if (PauseWidget)
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void ADCPlayerController::OpenPauseMenu()
{
	// not while another full-screen UI owns the mouse (the victory screen), and not twice
	if (PauseWidget || !IsLocalPlayerController() || bShowMouseCursor || IsPaused())
	{
		return;
	}

	TSubclassOf<UDCPauseMenuWidget> WidgetCls = PauseMenuWidgetClass ? PauseMenuWidgetClass : TSubclassOf<UDCPauseMenuWidget>(UDCPauseMenuWidget::StaticClass());
	PauseWidget = CreateWidget<UDCPauseMenuWidget>(this, WidgetCls);
	if (!PauseWidget)
	{
		return;
	}
	PauseWidget->AddToViewport(200);      // above the HUD and the 2D weapon layer
	PauseWidget->OnResume.AddUniqueDynamic(this, &ADCPlayerController::ClosePauseMenu);
	PauseWidget->OnQuit.AddUniqueDynamic(this, &ADCPlayerController::QuitFromPauseMenu);

	SetPause(true);

	FInputModeUIOnly Mode;
	if (UWidget* Focus = PauseWidget->GetInitialFocusWidget())
	{
		Mode.SetWidgetToFocus(Focus->TakeWidget());
	}
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

void ADCPlayerController::ClosePauseMenu()
{
	if (!PauseWidget)
	{
		return;
	}
	PauseWidget->RemoveFromParent();
	PauseWidget = nullptr;

	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ADCPlayerController::QuitFromPauseMenu()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
