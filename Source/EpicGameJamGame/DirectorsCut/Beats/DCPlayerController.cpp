// Director's Cut - player controller.

#include "Beats/DCPlayerController.h"
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

	if (UCheckpointSubsystem* Checkpoints = World->GetSubsystem<UCheckpointSubsystem>())
	{
		if (Checkpoints->HasActiveCheckpoint())
		{
			SpawnTransform = Checkpoints->GetRespawnTransform();
			bHaveSpawn = true;
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
