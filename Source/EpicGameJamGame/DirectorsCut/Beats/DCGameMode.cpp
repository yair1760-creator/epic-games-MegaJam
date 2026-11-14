// Director's Cut - game mode: full 5-beat flow.

#include "Beats/DCGameMode.h"
#include "Beats/DCPlayerController.h"
#include "Beats/DCPrincess.h"
#include "Beats/DCShedDoors.h"
#include "AI/DCBoar.h"
#include "AI/DCBoarSpawner.h"
#include "AI/DCBoarSubsystem.h"
#include "AI/DCDirectorBoss.h"
#include "AI/DCKingBoar.h"
#include "Combat/DCBoarMeatPickup.h"
#include "Combat/DCFXLibrary.h"
#include "Combat/DCHealthComponent.h"
#include "Combat/DCShotgunWeapon.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Narrator/DCNarratorSubsystem.h"
#include "Quest/DCQuestSubsystem.h"
#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/DCDeathScreenWidget.h"
#include "UI/DCHUDWidget.h"
#include "UI/DCVictoryWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "EpicGameJamGame.h"

ADCGameMode::ADCGameMode()
{
	DC_DEFAULT_SFX(CutSound, "S_Cut");
	PlayerControllerClass = ADCPlayerController::StaticClass();
	IntroLines = { FName("L1_Intro_01"), FName("L1_Intro_02"), FName("L1_Intro_03"), FName("L1_Intro_04") };
	HuntQuestTitle = NSLOCTEXT("DirectorsCut", "HuntQuestTitle", "Boars killed");

	static ConstructorHelpers::FClassFinder<AShooterWeapon> SwordBP(TEXT("/Game/DirectorsCut/Combat/Sword/BP_DC_Sword"));
	if (SwordBP.Succeeded())
	{
		SwordWeaponClass = SwordBP.Class;
	}
	GunWeaponClass = ADCShotgunWeapon::StaticClass();
	static ConstructorHelpers::FClassFinder<UDCDeathScreenWidget> DeathScreenBP(TEXT("/Game/DirectorsCut/UI/WBP_DeathScreen"));
	if (DeathScreenBP.Succeeded())
	{
		DeathScreenWidgetClass = DeathScreenBP.Class;
	}
	static ConstructorHelpers::FClassFinder<UDCVictoryWidget> CutScreenBP(TEXT("/Game/DirectorsCut/UI/WBP_CutScreen"));
	if (CutScreenBP.Succeeded())
	{
		VictoryWidgetClass = CutScreenBP.Class;
	}

	BeatSublevels.Add(1, { { FName("DC_Light_Day"),  FName("DC_Set_Hunt") } });
	BeatSublevels.Add(2, { { FName("DC_Light_Day"),  FName("DC_Set_Hunt"), FName("DC_Set_NoMeat") } });
	BeatSublevels.Add(3, { { FName("DC_Light_Day"),  FName("DC_Set_Hunt"), FName("DC_Set_FilmCrew") } });
	BeatSublevels.Add(4, { { FName("DC_Light_PG"),   FName("DC_Set_Hunt"), FName("DC_Set_PG") } });
	BeatSublevels.Add(5, { { FName("DC_Light_Boss"), FName("DC_Set_Boss") } });
}

void ADCGameMode::BeginPlay()
{
	Super::BeginPlay();
	RunStartTime = GetWorld()->GetTimeSeconds();

	if (HUDWidgetClass)
	{
		if ((HUDWidget = CreateWidget<UDCHUDWidget>(GetWorld()->GetFirstPlayerController(), HUDWidgetClass)))
		{
			HUDWidget->AddToViewport(0);
		}
	}

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->OnQuestUpdated.AddUniqueDynamic(this, &ADCGameMode::HandleQuestUpdated);
		Quest->OnQuestCompleted.AddUniqueDynamic(this, &ADCGameMode::HandleQuestCompleted);
	}

	if (UDCBoarSubsystem* Boars = GetWorld()->GetSubsystem<UDCBoarSubsystem>())
	{
		Boars->OnBoarKilled.AddUniqueDynamic(this, &ADCGameMode::HandleBoarKilled);
		Boars->OnBoarSpawned.AddUniqueDynamic(this, &ADCGameMode::HandleBoarSpawned);
	}

	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->OnLineStarted.AddUniqueDynamic(this, &ADCGameMode::HandleLineStarted);
		Narrator->OnLineFinished.AddUniqueDynamic(this, &ADCGameMode::HandleLineFinished);
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ADCGameMode::HandlePossessedPawnChanged);
		HandlePossessedPawnChanged(nullptr, PC->GetPawn());
	}

	UpdateMusicForBeat(CurrentBeat);
	GetWorldTimerManager().SetTimer(IntroTimer, this, &ADCGameMode::StartIntro, FMath::Max(IntroDelay, 0.01f), false);
}

void ADCGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(IntroTimer);
	GetWorldTimerManager().ClearTimer(IdleTimer);
	GetWorldTimerManager().ClearTimer(BossStartFallbackTimer);
	GetWorldTimerManager().ClearTimer(Beat5FallbackTimer);
	GetWorldTimerManager().ClearTimer(VictoryFallbackTimer);
	GetWorldTimerManager().ClearTimer(MusicUnduckTimer);
	StopBeatMusic(0.0f);
	if (VictoryMusicComponent)
	{
		VictoryMusicComponent->Stop();
		VictoryMusicComponent = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ADCGameMode::StartIntro()
{
	UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>();
	if (!Narrator || IntroLines.IsEmpty())
	{
		StartHuntQuest();
		return;
	}

	Narrator->PlaySequence(IntroLines);
}

void ADCGameMode::HandleLineStarted(FText Speaker, FText Text, float Duration)
{
	GetWorldTimerManager().ClearTimer(MusicUnduckTimer);
	if (!bMusicDucked)
	{
		bMusicDucked = true;
		RefreshMusicDucking();
	}
}

void ADCGameMode::HandleLineFinished(FName RowId)
{
	// Wait one tick before unducking so back-to-back queued lines stay smoothly ducked.
	MusicUnduckTimer = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UDCNarratorSubsystem* Narrator = GetWorld() ? GetWorld()->GetSubsystem<UDCNarratorSubsystem>() : nullptr;
		if (!Narrator || !Narrator->IsPlaying())
		{
			bMusicDucked = false;
			RefreshMusicDucking();
		}
	}));

	if (CurrentBeat == 1 && !bHuntQuestStarted && !IntroLines.IsEmpty() && RowId == IntroLines.Last())
	{
		StartHuntQuest();
		return;
	}

	if (bPendingBeat5 && RowId == FName("B4_PrincessRescued"))
	{
		bPendingBeat5 = false;
		GetWorldTimerManager().ClearTimer(Beat5FallbackTimer);
		EnterBeat5();
		return;
	}

	if (CurrentBeat == 5 && RowId == FName("B5_Intro_03"))
	{
		StartBossFightNow();
		return;
	}

	if (bVictoryPending && RowId == FName("B5_DirectorDefeated"))
	{
		ShowVictoryScene();
	}
}

void ADCGameMode::StartHuntQuest()
{
	if (bHuntQuestStarted)
	{
		return;
	}
	bHuntQuestStarted = true;
	CurrentBeat = 1;

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->StartQuest(HuntQuestId, HuntQuestTitle, HuntQuestTarget, HuntQuestEventTag);
	}

	int32 SpawnersStarted = 0;
	for (TActorIterator<ADCBoarSpawner> It(GetWorld()); It; ++It)
	{
		It->SetHyperspawnMode(false);
		It->StartSpawning();
		++SpawnersStarted;
	}

	ResetIdleTimer();
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Beat 1: intro done, hunt quest started (%d boar spawner(s) started)"), SpawnersStarted);
}

void ADCGameMode::EnterBeat2()
{
	if (CurrentBeat == 2)
	{
		return;
	}
	CurrentBeat = 2;
	Beat2BoarKillCount = 0;
	GetWorldTimerManager().ClearTimer(IdleTimer);

	GoToBeat(2);
	EquipBeatWeapon(false);

	for (TActorIterator<ADCBoarSpawner> It(GetWorld()); It; ++It)
	{
		It->SetHyperspawnMode(false);
		It->StartSpawning();
	}

	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->Interrupt();
		Narrator->PlayLine(FName("B2_Intro_01"));
		Narrator->PlayAlternatingLine(FName("B2_Intro_02"), FName("B2_Intro_02_Alt"));
	}

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReplaceQuest(
			FName("Q_CollectMeat"),
			NSLOCTEXT("DirectorsCut", "CollectMeatTitle", "Bacon collected"),
			5,
			FName("Collect.Meat"),
			false);
	}
}

void ADCGameMode::EnterBeat3()
{
	if (CurrentBeat == 3)
	{
		return;
	}
	CurrentBeat = 3;
	GetWorldTimerManager().ClearTimer(IdleTimer);

	// Clean up any leftover meat pickups from Beat 2
	for (TActorIterator<ADCBoarMeatPickup> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	GoToBeat(3);
	EquipBeatWeapon(true);

	for (TActorIterator<ADCBoarSpawner> It(GetWorld()); It; ++It)
	{
		It->SetHyperspawnMode(true);
	}

	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->Interrupt();
		Narrator->PlayLine(FName("B3_Intro_01"));
		Narrator->PlayLine(FName("B3_Intro_02"));
		Narrator->PlayAlternatingLine(FName("B3_Intro_03"), FName("B3_Intro_03_Alt"));
	}

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReplaceQuest(
			FName("Q_HyperspawnBoars"),
			NSLOCTEXT("DirectorsCut", "HyperspawnTitle", "Boars eliminated"),
			100,
			FName("Kill.Boar"),
			false);
	}
}

void ADCGameMode::EnterBeat4()
{
	if (CurrentBeat == 4)
	{
		return;
	}
	CurrentBeat = 4;
	bPendingBeat5 = false;
	GetWorldTimerManager().ClearTimer(IdleTimer);
	CleanupBeatActors();

	for (TActorIterator<ADCBoarSpawner> It(GetWorld()); It; ++It)
	{
		It->SetHyperspawnMode(false);
		It->StopSpawning(true);
	}

	GoToBeat(4);
	EquipBeatWeapon(false);

	// Use an already-placed ADCPrincess in the level (e.g. inside a shed or tower), or a TargetPoint tagged "PrincessSpawn",
	// falling back to the default clearing location if neither is placed yet.
	ADCPrincess* Princess = nullptr;
	for (TActorIterator<ADCPrincess> It(GetWorld()); It; ++It)
	{
		Princess = *It;
		break;
	}
	if (!Princess)
	{
		FVector PrincessLoc(3000.0f, -2400.0f, 0.0f);
		FRotator PrincessRot(0.0f, 140.0f, 0.0f);
		TArray<AActor*> PrincessMarkers;
		UGameplayStatics::GetAllActorsWithTag(this, FName("PrincessSpawn"), PrincessMarkers);
		if (PrincessMarkers.Num() > 0 && PrincessMarkers[0])
		{
			PrincessLoc = PrincessMarkers[0]->GetActorLocation();
			PrincessRot = PrincessMarkers[0]->GetActorRotation();
		}

		FActorSpawnParameters PrincessParams;
		PrincessParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Princess = GetWorld()->SpawnActor<ADCPrincess>(ADCPrincess::StaticClass(), PrincessLoc, PrincessRot, PrincessParams);
	}
	if (Princess)
	{
		Princess->OnLockedAttempt.AddUniqueDynamic(this, &ADCGameMode::HandlePrincessLockedAttempt);
		ActivePrincess = Princess;
	}

	// Reset and bind any lockable shed doors in DC_Set_PG
	for (TActorIterator<ADCShedDoors> It(GetWorld()); It; ++It)
	{
		It->ResetLocked();
		It->OnLockedAttempt.AddUniqueDynamic(this, &ADCGameMode::HandlePrincessLockedAttempt);
	}

	// Same for King Boar: use a "KingBoarSpawn" marker if placed, otherwise guard just in front of the Princess.
	FVector KingLoc = Princess
		? Princess->GetActorLocation() + Princess->GetActorForwardVector() * 750.0f + FVector(0.0f, 0.0f, 130.0f)
		: FVector(2350.0f, -1850.0f, 130.0f);
	FRotator KingRot = Princess ? Princess->GetActorRotation() : FRotator(0.0f, 140.0f, 0.0f);
	TArray<AActor*> KingMarkers;
	UGameplayStatics::GetAllActorsWithTag(this, FName("KingBoarSpawn"), KingMarkers);
	if (KingMarkers.Num() > 0 && KingMarkers[0])
	{
		KingLoc = KingMarkers[0]->GetActorLocation() + FVector(0.0f, 0.0f, 130.0f);
		KingRot = KingMarkers[0]->GetActorRotation();
	}

	FActorSpawnParameters KingParams;
	KingParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ADCKingBoar* King = GetWorld()->SpawnActor<ADCKingBoar>(ADCKingBoar::StaticClass(), KingLoc, KingRot, KingParams);
	if (King)
	{
		ActiveKingBoar = King;
	}

	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->Interrupt();
		Narrator->PlaySequence({ FName("B4_Intro_01"), FName("B4_Intro_02"), FName("B4_Intro_03") });
	}

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReplaceQuest(
			FName("Q_DefeatKingBoar"),
			NSLOCTEXT("DirectorsCut", "KingBoarTitle", "Defeat the King Boar"),
			1,
			FName("Kill.KingBoar"),
			false);
	}
}

void ADCGameMode::EnterBeat5()
{
	if (CurrentBeat == 5)
	{
		return;
	}
	CurrentBeat = 5;
	bPendingBeat5 = false;
	GetWorldTimerManager().ClearTimer(IdleTimer);
	GetWorldTimerManager().ClearTimer(Beat5FallbackTimer);
	CleanupBeatActors();

	for (TActorIterator<ADCBoarSpawner> It(GetWorld()); It; ++It)
	{
		It->SetHyperspawnMode(false);
		It->StopSpawning(true);
	}

	GoToBeat(5);
	EquipBeatWeapon(false);

	// Place player cleanly inside the Beat 5 arena facing The Director
	FTransform ArenaSpawn;
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && GetBeatRespawnTransform(ArenaSpawn))
	{
		Player->SetActorLocationAndRotation(ArenaSpawn.GetLocation(), ArenaSpawn.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
		if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
		{
			PC->SetControlRotation(ArenaSpawn.Rotator());
		}
	}

	FActorSpawnParameters BossParams;
	BossParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ADCDirectorBoss* Boss = GetWorld()->SpawnActor<ADCDirectorBoss>(ADCDirectorBoss::StaticClass(), FVector(2200.0f, 0.0f, 150.0f), FRotator(0.0f, 180.0f, 0.0f), BossParams);
	if (Boss)
	{
		Boss->OnBossHalfHP.AddUniqueDynamic(this, &ADCGameMode::HandleBossHalfHP);
		Boss->OnBossTaunt.AddUniqueDynamic(this, &ADCGameMode::HandleBossTaunt);
		ActiveDirectorBoss = Boss;
	}

	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->Interrupt();
		Narrator->PlaySequence({ FName("B5_Intro_01"), FName("B5_Intro_02"), FName("B5_Intro_03") });
	}

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReplaceQuest(
			FName("Q_DefeatDirector"),
			NSLOCTEXT("DirectorsCut", "BossTitle", "Defeat the Director"),
			1,
			FName("Kill.Director"),
			false);
	}

	// Safety fallback in case VO is interrupted or missing (B5_Intro_01..03 total ~21.3s)
	GetWorldTimerManager().SetTimer(BossStartFallbackTimer, this, &ADCGameMode::StartBossFightNow, 25.0f, false);
}

void ADCGameMode::StartBossFightNow()
{
	GetWorldTimerManager().ClearTimer(BossStartFallbackTimer);
	if (ADCDirectorBoss* Boss = ActiveDirectorBoss.Get())
	{
		Boss->StartFight();
	}
}

void ADCGameMode::CleanupBeatActors()
{
	if (ADCPrincess* Princess = ActivePrincess.Get())
	{
		Princess->Destroy();
	}
	ActivePrincess.Reset();

	if (ADCKingBoar* King = ActiveKingBoar.Get())
	{
		King->Destroy();
	}
	ActiveKingBoar.Reset();

	if (ADCDirectorBoss* Boss = ActiveDirectorBoss.Get())
	{
		Boss->Destroy();
	}
	ActiveDirectorBoss.Reset();

	for (TActorIterator<ADCBoarMeatPickup> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
}

void ADCGameMode::StartBeat(int32 Beat)
{
	const int32 Clamped = FMath::Clamp(Beat, 1, 5);
	GetWorldTimerManager().ClearTimer(IntroTimer);
	GetWorldTimerManager().ClearTimer(IdleTimer);
	GetWorldTimerManager().ClearTimer(BossStartFallbackTimer);
	GetWorldTimerManager().ClearTimer(Beat5FallbackTimer);
	GetWorldTimerManager().ClearTimer(VictoryFallbackTimer);

	bVictoryPending = false;
	bVictoryShown = false;
	HideDeathScreen();
	if (Clamped == 1)
	{
		CurrentTakeNumber = 1;
	}
	bMusicDucked = false;
	GetWorldTimerManager().ClearTimer(MusicUnduckTimer);
	if (VictoryMusicComponent)
	{
		VictoryMusicComponent->Stop();
		VictoryMusicComponent = nullptr;
	}
	if (VictoryWidget)
	{
		VictoryWidget->RemoveFromParent();
		VictoryWidget = nullptr;
	}
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ADCPlayerController* DCPC = Cast<ADCPlayerController>(PC))
		{
			DCPC->ClosePauseMenu();
		}
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Player->SetCanBeDamaged(true);
	}

	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->Interrupt();
	}

	if (Clamped == 1)
	{
		CleanupBeatActors();
		bHuntQuestStarted = false;
		bPlayedFirstHit = false;
		bPlayedPlayerHurt = false;
		GoToBeat(1);
		EquipBeatWeapon(false);
		StartHuntQuest();
		return;
	}

	// Prevent CurrentBeat == Clamped early-return when jumping directly via console command
	CurrentBeat = 0;
	switch (Clamped)
	{
	case 2: EnterBeat2(); break;
	case 3: EnterBeat3(); break;
	case 4: EnterBeat4(); break;
	case 5: EnterBeat5(); break;
	default: break;
	}
}

void ADCGameMode::HandleQuestUpdated(FName Id, int32 Current, int32 Target)
{
	UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>();

	if (Id == HuntQuestId)
	{
		if (Current > 0)
		{
			ResetIdleTimer();
		}
		if (Current == 2 && Narrator)
		{
			Narrator->PlayLine(FName("L1_FirstKill"));
		}
		else if (Current == 5 && Narrator)
		{
			Narrator->PlayLine(FName("L1_Half"));
		}
		else if (Current >= 7 && CurrentBeat == 1)
		{
			// After 7 boars, the Director sees meat drop, gets hungry for bacon, and switches to Beat 2!
			EnterBeat2();
		}
		return;
	}

	if (Id == FName("Q_CollectMeat") && Narrator)
	{
		if (Current == 2 && !Narrator->IsPlaying())
		{
			Narrator->PlayLine(FName("B2_FirstMeat"));
		}
		else if (Current == 4)
		{
			Narrator->PlayLine(FName("B2_AlmostDone"));
		}
		return;
	}

	if (Id == FName("Q_HyperspawnBoars") && Narrator)
	{
		if (Current == 25 && !Narrator->IsPlaying())
		{
			Narrator->PlayLine(FName("B3_Progress_25"));
		}
		else if (Current == 50 && !Narrator->IsPlaying())
		{
			Narrator->PlayLine(FName("B3_Progress_50"));
		}
		else if (Current == 80 && !Narrator->IsPlaying())
		{
			Narrator->PlayLine(FName("B3_Progress_80"));
		}
	}
}

void ADCGameMode::HandleQuestCompleted(FName Id)
{
	if (Id == FName("Q_CollectMeat") && CurrentBeat == 2)
	{
		EnterBeat3();
	}
	else if (Id == FName("Q_HyperspawnBoars") && CurrentBeat == 3)
	{
		EnterBeat4();
	}
	else if (Id == FName("Q_DefeatKingBoar") && CurrentBeat == 4)
	{
		if (ADCPrincess* Princess = ActivePrincess.Get())
		{
			Princess->UnlockPrincess();
		}
		for (TActorIterator<ADCShedDoors> It(GetWorld()); It; ++It)
		{
			It->UnlockAndOpen();
		}
		if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
		{
			Narrator->Interrupt();
			Narrator->PlayLine(FName("B4_KingBoarDead"));
		}
		if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
		{
			Quest->ReplaceQuest(
				FName("Q_RescuePrincess"),
				NSLOCTEXT("DirectorsCut", "RescuePrincessTitle", "Rescue the Princess"),
				1,
				FName("Rescue.Princess"),
				false);
		}
	}
	else if (Id == FName("Q_RescuePrincess") && CurrentBeat == 4)
	{
		bPendingBeat5 = true;
		if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
		{
			Narrator->Interrupt();
			Narrator->PlayLine(FName("B4_PrincessRescued"));
		}
		GetWorldTimerManager().SetTimer(Beat5FallbackTimer, this, &ADCGameMode::EnterBeat5, 11.0f, false);
	}
	else if (Id == FName("Q_DefeatDirector") && CurrentBeat == 5)
	{
		bVictoryPending = true;
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			Player->SetCanBeDamaged(false);
		}
		if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
		{
			Narrator->Interrupt();
			Narrator->PlayLine(FName("B5_DirectorDefeated"));
		}
		GetWorldTimerManager().SetTimer(VictoryFallbackTimer, this, &ADCGameMode::ShowVictoryScene, VictoryDelayAfterDefeat, false);
	}
}

void ADCGameMode::HandleBoarKilled(ADCBoar* Boar, APawn* Killer)
{
	if (!Boar || Boar->IsA<ADCKingBoar>())
	{
		return;
	}

	if (CurrentBeat == 2)
	{
		++Beat2BoarKillCount;
		// Kill #3 (and every 4th after) drops NO meat -> triggers the "this one got no meat" joke!
		const bool bNoMeatJoke = (Beat2BoarKillCount == 3) || (Beat2BoarKillCount > 3 && (Beat2BoarKillCount % 4) == 3);
		if (bNoMeatJoke)
		{
			if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
			{
				Narrator->PlayAlternatingLine(FName("B2_NoMeat_01"), FName("B2_NoMeat_02"));
			}
		}
		else
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<ADCBoarMeatPickup>(
				ADCBoarMeatPickup::StaticClass(),
				Boar->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f),
				FRotator::ZeroRotator,
				Params);
		}
	}
}

void ADCGameMode::HandleBoarSpawned(ADCBoar* Boar)
{
	if (Boar && !bPlayedFirstHit)
	{
		if (UDCHealthComponent* Health = Boar->GetHealth())
		{
			Health->OnHealthChanged.AddUniqueDynamic(this, &ADCGameMode::HandleBoarFirstDamaged);
		}
	}
}

void ADCGameMode::HandleBoarFirstDamaged(UDCHealthComponent* HealthComp, float CurrentHP, float Delta, AActor* DamageCauser)
{
	if (!bPlayedFirstHit && Delta < 0.0f && CurrentBeat == 1 && bHuntQuestStarted)
	{
		bPlayedFirstHit = true;
		ResetIdleTimer();
		if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
		{
			Narrator->PlayLine(FName("L1_FirstHit"));
		}
	}
}

void ADCGameMode::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (AShooterCharacter* OldChar = BoundPlayerCharacter.Get())
	{
		OldChar->OnHealthChanged.RemoveAll(this);
		OldChar->OnDied.RemoveAll(this);
	}
	BoundPlayerCharacter.Reset();

	if (AShooterCharacter* NewChar = Cast<AShooterCharacter>(NewPawn))
	{
		BoundPlayerCharacter = NewChar;
		NewChar->OnHealthChanged.AddUniqueDynamic(this, &ADCGameMode::HandlePlayerHealthChanged);
		NewChar->OnDied.AddUniqueDynamic(this, &ADCGameMode::HandlePlayerDied);
		bPlayedPlayerHurt = false;

		if (!NewChar->IsDead())
		{
			HideDeathScreen();
		}

		if (bVictoryPending || bVictoryShown)
		{
			NewChar->SetCanBeDamaged(false);
		}

		if (CurrentBeat == 3)
		{
			// Apply on next tick so AShooterCharacter::BeginPlay's default sword doesn't override the shotgun
			GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				EquipBeatWeapon(CurrentBeat == 3);
			}));
		}
	}
}

void ADCGameMode::HandlePlayerHealthChanged(float CurrentHP, float MaxHP, float Delta)
{
	if (!bPlayedPlayerHurt && CurrentBeat < 5 && (CurrentBeat != 1 || bHuntQuestStarted)
		&& Delta < 0.0f && MaxHP > 0.0f && (CurrentHP / MaxHP) < 0.5f && CurrentHP > 0.0f)
	{
		if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
		{
			if (!Narrator->IsPlaying())
			{
				bPlayedPlayerHurt = true;
				Narrator->PlayLine(FName("L1_PlayerHurt"));
			}
		}
	}
}

void ADCGameMode::HandlePlayerDied(AShooterCharacter* Character)
{
	if (bVictoryPending || bVictoryShown)
	{
		return;
	}
	if (ADCPlayerController* DCPC = Cast<ADCPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		DCPC->ClosePauseMenu();
	}
	++CurrentTakeNumber;
	ShowDeathScreen(Character ? Character->GetRespawnTime() : 5.0f);
	if (USoundBase* CutSFX = CutSound ? CutSound.Get() : (FPackageName::DoesPackageExist(TEXT("/Game/DirectorsCut/Audio/SFX/S_Cut")) ? LoadObject<USoundBase>(nullptr, TEXT("/Game/DirectorsCut/Audio/SFX/S_Cut")) : nullptr))
	{
		UDCFXLibrary::PlayUISound(this, CutSFX, 0.75f);
	}
	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->Interrupt();
		Narrator->PlayLine(FName("L1_PlayerDied"));
	}
	if (CurrentBeat == 1 && !bHuntQuestStarted)
	{
		StartHuntQuest();
	}
}

void ADCGameMode::ShowDeathScreen(float RespawnDuration)
{
	if (bVictoryShown)
	{
		return;
	}

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	bDeathScreenShown = true;
	if (HUDWidget)
	{
		HUDWidget->SetCrosshairVisible(false);
		HUDWidget->RefreshTakeLabel(CurrentBeat);
	}

	if (!DeathScreenWidget)
	{
		TSubclassOf<UDCDeathScreenWidget> WidgetCls = DeathScreenWidgetClass ? DeathScreenWidgetClass : TSubclassOf<UDCDeathScreenWidget>(UDCDeathScreenWidget::StaticClass());
		DeathScreenWidget = CreateWidget<UDCDeathScreenWidget>(PC, WidgetCls);
	}
	if (DeathScreenWidget)
	{
		if (!DeathScreenWidget->IsInViewport())
		{
			// Sit above DCScreenTintWidget (-5) and below WBP_DCHUD (0) so live subtitles + 0 HP bar remain visible.
			DeathScreenWidget->AddToViewport(-2);
		}
		DeathScreenWidget->SetupDeath(CurrentTakeNumber, GetBeatDisplayName(CurrentBeat), RespawnDuration);
	}
}

void ADCGameMode::HideDeathScreen()
{
	bDeathScreenShown = false;
	if (DeathScreenWidget)
	{
		DeathScreenWidget->RemoveFromParent();
		DeathScreenWidget = nullptr;
	}
	if (HUDWidget)
	{
		HUDWidget->SetCrosshairVisible(true);
		HUDWidget->RefreshTakeLabel(CurrentBeat);
	}
}

void ADCGameMode::HandlePrincessLockedAttempt()
{
	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		if (!Narrator->IsPlaying())
		{
			Narrator->PlayLine(FName("B4_ShedLocked"));
		}
	}
}

void ADCGameMode::HandleBossHalfHP()
{
	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		Narrator->Interrupt();
		Narrator->PlayLine(FName("B5_BossHalfHP"));
	}
}

void ADCGameMode::HandleBossTaunt()
{
	if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
	{
		if (!Narrator->IsPlaying())
		{
			Narrator->PlayAlternatingLine(FName("B5_BossTaunt_01"), FName("B5_BossTaunt_02"));
		}
	}
}

void ADCGameMode::ResetIdleTimer()
{
	if (CurrentBeat == 1)
	{
		GetWorldTimerManager().SetTimer(IdleTimer, this, &ADCGameMode::HandleIdleTimeout, 30.0f, false);
	}
}

void ADCGameMode::HandleIdleTimeout()
{
	if (CurrentBeat == 1)
	{
		if (UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>())
		{
			if (!Narrator->IsPlaying())
			{
				Narrator->PlayLine(FName("L1_Idle"));
			}
		}
	}
}

bool ADCGameMode::GetBeatRespawnTransform(FTransform& OutTransform) const
{
	if (CurrentBeat != 5)
	{
		return false;
	}

	TArray<AActor*> ArenaMarkers;
	UGameplayStatics::GetAllActorsWithTag(this, FName("BossArenaSpawn"), ArenaMarkers);
	if (ArenaMarkers.Num() > 0 && ArenaMarkers[0])
	{
		OutTransform = ArenaMarkers[0]->GetActorTransform();
		return true;
	}

	OutTransform = FTransform(Beat5ArenaSpawnRotation, Beat5ArenaSpawnLocation);
	return true;
}

void ADCGameMode::ShowVictoryScene()
{
	if (bVictoryShown)
	{
		return;
	}
	bVictoryShown = true;
	bVictoryPending = false;
	HideDeathScreen();

	GetWorldTimerManager().ClearTimer(IdleTimer);
	GetWorldTimerManager().ClearTimer(BossStartFallbackTimer);
	GetWorldTimerManager().ClearTimer(VictoryFallbackTimer);

	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (ADCPlayerController* DCPC = Cast<ADCPlayerController>(PC))
	{
		DCPC->ClosePauseMenu();
	}
	if (!VictoryWidget && PC)
	{
		TSubclassOf<UDCVictoryWidget> WidgetCls = VictoryWidgetClass ? VictoryWidgetClass : TSubclassOf<UDCVictoryWidget>(UDCVictoryWidget::StaticClass());
		VictoryWidget = CreateWidget<UDCVictoryWidget>(PC, WidgetCls);
		if (VictoryWidget)
		{
			VictoryWidget->AddToViewport(100);
		}
	}

	if (VictoryWidget)
	{
		int32 BoarsKilled = 0;
		if (UDCBoarSubsystem* Boars = GetWorld()->GetSubsystem<UDCBoarSubsystem>())
		{
			BoarsKilled = Boars->GetKilledCount();
		}
		const float Elapsed = FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - RunStartTime);
		VictoryWidget->SetupVictory(BoarsKilled, Elapsed);
	}

	if (PC)
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		FInputModeUIOnly Mode;
		if (VictoryWidget)
		{
			if (UWidget* FocusTarget = VictoryWidget->GetInitialFocusWidget())
			{
				Mode.SetWidgetToFocus(FocusTarget->TakeWidget());
			}
		}
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}

	auto LoadOptionalSound = [](TObjectPtr<USoundBase> Assigned, const TCHAR* PackagePath) -> USoundBase*
	{
		if (Assigned)
		{
			return Assigned.Get();
		}
		if (FPackageName::DoesPackageExist(PackagePath))
		{
			return LoadObject<USoundBase>(nullptr, PackagePath);
		}
		return nullptr;
	};

	if (USoundBase* CutSFX = LoadOptionalSound(CutSound, TEXT("/Game/DirectorsCut/Audio/SFX/S_Cut")))
	{
		UDCFXLibrary::PlayUISound(this, CutSFX, 1.0f);
	}
	StopBeatMusic(0.4f);
	if (USoundBase* VictoryCue = LoadOptionalSound(VictorySound, TEXT("/Game/DirectorsCut/Audio/Music/M_Victory")))
	{
		if (VictoryMusicComponent)
		{
			VictoryMusicComponent->Stop();
			VictoryMusicComponent = nullptr;
		}
		VictoryMusicComponent = UGameplayStatics::SpawnSound2D(this, VictoryCue, 0.9f);
	}

	OnVictoryTriggered.Broadcast();
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Victory scene displayed (THAT'S A WRAP!)"));
}

FString ADCGameMode::GetBeatDisplayName(int32 Beat) const
{
	switch (Beat)
	{
	case 1: return TEXT("SCENE 1: BOAR HUNT");
	case 2: return TEXT("SCENE 2: BREAKFAST");
	case 3: return TEXT("SCENE 3: ACTION BLOCKBUSTER");
	case 4: return TEXT("SCENE 4: RATED PG ROMANCE");
	case 5: return TEXT("SCENE 5: FINAL CUT");
	default: return FString::Printf(TEXT("SCENE %d"), Beat);
	}
}

void ADCGameMode::EquipBeatWeapon(bool bUseGun)
{
	AShooterCharacter* PlayerChar = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!PlayerChar)
	{
		return;
	}

	TSubclassOf<AShooterWeapon> Desired = bUseGun ? GunWeaponClass : SwordWeaponClass;
	if (!Desired)
	{
		return;
	}

	if (AShooterWeapon* Cur = PlayerChar->GetCurrentWeapon(); Cur && Cur->IsA(Desired))
	{
		return;
	}

	PlayerChar->ReplaceWeaponsWithClass(Desired);
}

void ADCGameMode::GoToBeat(int32 Beat)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FDCBeatSublevels* Entry = BeatSublevels.Find(Beat);
	if (!Entry)
	{
		UE_LOG(LogEpicGameJamGame, Warning, TEXT("GoToBeat: no sublevel list for beat %d"), Beat);
		return;
	}

	TSet<ULevelStreaming*> Desired;
	for (const FName& Name : Entry->Sublevels)
	{
		if (ULevelStreaming* SL = UGameplayStatics::GetStreamingLevel(World, Name))
		{
			Desired.Add(SL);
		}
		else
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("GoToBeat(%d): streaming sublevel '%s' not found in %s"), Beat, *Name.ToString(), *World->GetName());
		}
	}

	// Unload unwanted sublevels first (especially old DC_Light_*), then load wanted ones.
	for (ULevelStreaming* SL : World->GetStreamingLevels())
	{
		if (!SL || !SL->GetWorldAssetPackageName().Contains(TEXT("/DirectorsCut/Maps/")))
		{
			continue;
		}
		if (!Desired.Contains(SL))
		{
			SL->SetShouldBeVisible(false);
			SL->SetShouldBeLoaded(false);
		}
	}

	for (ULevelStreaming* SL : Desired)
	{
		SL->SetShouldBeLoaded(true);
		SL->SetShouldBeVisible(true);
	}

	World->FlushLevelStreaming(EFlushLevelStreamingType::Full);

	CurrentBeat = Beat;
	UpdateMusicForBeat(Beat);
	UE_LOG(LogEpicGameJamGame, Log, TEXT("GoToBeat(%d): %d sublevel(s) active"), Beat, Desired.Num());
	OnBeatChanged.Broadcast(Beat);
}

USoundBase* ADCGameMode::ResolveMusicForBeat(int32 Beat) const
{
	if (const TObjectPtr<USoundBase>* Override = BeatMusic.Find(Beat); Override && *Override)
	{
		return Override->Get();
	}

	const TCHAR* DefaultPath = nullptr;
	switch (Beat)
	{
	case 1:
	case 2:
		DefaultPath = TEXT("/Game/DirectorsCut/Audio/Music/M_Heroic");
		break;
	case 3:
		DefaultPath = TEXT("/Game/DirectorsCut/Audio/Music/M_Frantic");
		break;
	case 4:
		DefaultPath = TEXT("/Game/DirectorsCut/Audio/Music/M_Romance");
		break;
	case 5:
		DefaultPath = TEXT("/Game/DirectorsCut/Audio/Music/M_Boss");
		break;
	default:
		break;
	}

	if (DefaultPath && FPackageName::DoesPackageExist(DefaultPath))
	{
		return LoadObject<USoundBase>(nullptr, DefaultPath);
	}
	return nullptr;
}

float ADCGameMode::GetTargetMusicVolume() const
{
	return FMath::Max(0.0f, MusicVolume * (bMusicDucked ? MusicDuckScale : 1.0f));
}

void ADCGameMode::RefreshMusicDucking()
{
	if (ActiveMusicComponent && ActiveMusicComponent->IsActive())
	{
		ActiveMusicComponent->AdjustVolume(MusicDuckFadeDuration, GetTargetMusicVolume());
	}
}

void ADCGameMode::StopBeatMusic(float FadeOutSeconds)
{
	if (ActiveMusicComponent)
	{
		if (FadeOutSeconds > 0.0f && ActiveMusicComponent->IsActive())
		{
			ActiveMusicComponent->FadeOut(FadeOutSeconds, 0.0f);
		}
		else
		{
			ActiveMusicComponent->Stop();
		}
		ActiveMusicComponent = nullptr;
	}
	ActiveMusicAsset = nullptr;
}

void ADCGameMode::UpdateMusicForBeat(int32 Beat)
{
	USoundBase* DesiredMusic = ResolveMusicForBeat(Beat);
	if (DesiredMusic == ActiveMusicAsset && DesiredMusic != nullptr)
	{
		// Beats 1 and 2 share M_Heroic; keep it playing seamlessly across the transition.
		return;
	}

	StopBeatMusic(MusicFadeDuration);
	ActiveMusicAsset = DesiredMusic;
	if (!DesiredMusic || !GetWorld())
	{
		return;
	}

	if (UAudioComponent* NewComp = UGameplayStatics::CreateSound2D(this, DesiredMusic, 1.0f, 1.0f, 0.0f, nullptr, false, true))
	{
		NewComp->FadeIn(MusicFadeDuration, GetTargetMusicVolume());
		ActiveMusicComponent = NewComp;
	}
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Beat %d music -> %s"), Beat, *DesiredMusic->GetName());
}

void ADCGameMode::SetSublevelLoaded(FName SublevelName, bool bLoaded)
{
	UWorld* World = GetWorld();
	if (!World || SublevelName.IsNone())
	{
		return;
	}

	if (ULevelStreaming* SL = UGameplayStatics::GetStreamingLevel(World, SublevelName))
	{
		SL->SetShouldBeLoaded(bLoaded);
		SL->SetShouldBeVisible(bLoaded);
		World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
		UE_LOG(LogEpicGameJamGame, Log, TEXT("SetSublevelLoaded(%s, %s)"), *SublevelName.ToString(), bLoaded ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogEpicGameJamGame, Warning, TEXT("SetSublevelLoaded: streaming sublevel '%s' not found"), *SublevelName.ToString());
	}
}

// ---------------------------------------------------------------- debug console commands (PIE)

static ADCGameMode* FindDCGameMode(UWorld* World)
{
	if (ADCGameMode* GM = World ? World->GetAuthGameMode<ADCGameMode>() : nullptr)
	{
		return GM;
	}
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* W = Context.World(); W && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				if (ADCGameMode* GM = W->GetAuthGameMode<ADCGameMode>())
				{
					return GM;
				}
			}
		}
	}
	UE_LOG(LogEpicGameJamGame, Warning, TEXT("DC.GoToBeat: start Play first"));
	return nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs GDCGoToBeatCmd(
	TEXT("DC.GoToBeat"),
	TEXT("DC.GoToBeat <1..5> - switch sublevels, quests, weapons, and Director beat state"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("Usage: DC.GoToBeat <1..5>"));
			return;
		}
		if (ADCGameMode* GM = FindDCGameMode(World))
		{
			GM->StartBeat(FCString::Atoi(*Args[0]));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDCSwapWeaponCmd(
	TEXT("DC.SwapWeapon"),
	TEXT("DC.SwapWeapon <Sword|Gun> - equip the Sword (Beats 1-2, 4-5) or Beat 3 Shotgun"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("Usage: DC.SwapWeapon <Sword|Gun>"));
			return;
		}
		if (ADCGameMode* GM = FindDCGameMode(World))
		{
			const bool bUseGun = Args[0].Equals(TEXT("Gun"), ESearchCase::IgnoreCase);
			GM->EquipBeatWeapon(bUseGun);
		}
	}));

static FAutoConsoleCommandWithWorld GDCVictoryCmd(
	TEXT("DC.Victory"),
	TEXT("DC.Victory - immediately show the Beat 5 victory wrap screen"),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (ADCGameMode* GM = FindDCGameMode(World))
		{
			GM->ShowVictoryScene();
		}
	}));
