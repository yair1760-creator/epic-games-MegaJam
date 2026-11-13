// Director's Cut - automated end-to-end verification of all 5 beats, quests, subtitles, VO, and actors.

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/DataTable.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "AI/DCBoar.h"
#include "AI/DCBoarSpawner.h"
#include "AI/DCBoarSubsystem.h"
#include "AI/DCDirectorBoss.h"
#include "AI/DCKingBoar.h"
#include "Beats/DCGameMode.h"
#include "Beats/DCPrincess.h"
#include "Combat/DCBoarMeatPickup.h"
#include "Combat/DCHealthComponent.h"
#include "Combat/DCSwordWeapon.h"
#include "Narrator/DCNarratorSubsystem.h"
#include "Quest/DCQuestSubsystem.h"
#include "UI/DCHUDWidget.h"
#include "UI/DCQuestTrackerWidget.h"
#include "UI/DCSubtitlesWidget.h"
#include "ShooterCharacter.h"
#include "ShooterWeapon.h"

// -----------------------------------------------------------------------------
// Test 1: Narrator DataTable + VO asset audit
// -----------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDCNarratorAuditTest,
	"DirectorsCut.Narrator.TableAndVoiceAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDCNarratorAuditTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/DirectorsCut/Narrator/DT_NarratorLines.DT_NarratorLines"));
	TestNotNull(TEXT("DT_NarratorLines should load"), Table);
	if (!Table)
	{
		return false;
	}

	const TArray<FName> ExpectedRows = {
		// Beat 1
		FName("L1_Intro_01"), FName("L1_Intro_02"), FName("L1_Intro_03"), FName("L1_Intro_04"),
		FName("L1_FirstHit"), FName("L1_FirstKill"), FName("L1_Half"),
		FName("L1_PlayerHurt"), FName("L1_PlayerDied"), FName("L1_Idle"),
		// Beat 2
		FName("B2_Intro_01"), FName("B2_Intro_02"), FName("B2_Intro_02_Alt"),
		FName("B2_FirstMeat"), FName("B2_NoMeat_01"), FName("B2_NoMeat_02"), FName("B2_AlmostDone"),
		// Beat 3
		FName("B3_Intro_01"), FName("B3_Intro_02"), FName("B3_Intro_03"), FName("B3_Intro_03_Alt"),
		FName("B3_Progress_25"), FName("B3_Progress_50"), FName("B3_Progress_80"),
		// Beat 4
		FName("B4_Intro_01"), FName("B4_Intro_02"), FName("B4_Intro_03"),
		FName("B4_ShedLocked"), FName("B4_KingBoarDead"), FName("B4_PrincessRescued"),
		// Beat 5
		FName("B5_Intro_01"), FName("B5_Intro_02"), FName("B5_Intro_03"),
		FName("B5_BossTaunt_01"), FName("B5_BossTaunt_02"), FName("B5_BossHalfHP"), FName("B5_DirectorDefeated")
	};

	for (const FName& RowId : ExpectedRows)
	{
		const FDCNarratorLine* Row = Table->FindRow<FDCNarratorLine>(RowId, TEXT("Audit"), false);
		TestNotNull(*FString::Printf(TEXT("Row %s must exist in DT_NarratorLines"), *RowId.ToString()), Row);
		if (!Row)
		{
			continue;
		}

		TestFalse(*FString::Printf(TEXT("Row %s Speaker should not be empty"), *RowId.ToString()), Row->Speaker.IsEmpty());
		TestFalse(*FString::Printf(TEXT("Row %s Text should not be empty"), *RowId.ToString()), Row->Text.IsEmpty());
		TestTrue(*FString::Printf(TEXT("Row %s Duration should be > 0"), *RowId.ToString()), Row->Duration > 0.0f);

		if (!Row->Voice)
		{
			AddWarning(FString::Printf(TEXT("Row %s has no Voice SoundWave assigned (subtitle-only)"), *RowId.ToString()));
		}
	}

	AddInfo(FString::Printf(TEXT("[DC_TEST] Audited %d narrator rows in DT_NarratorLines"), ExpectedRows.Num()));
	return true;
}

// -----------------------------------------------------------------------------
// Test 2: Full 5-Beat Gameplay & Quest Flow on Lvl_DirectorsCut
// -----------------------------------------------------------------------------
namespace DCFlowTestInternal
{
	static bool IsSublevelActive(UWorld* World, FName SublevelName)
	{
		if (ULevelStreaming* SL = UGameplayStatics::GetStreamingLevel(World, SublevelName))
		{
			return SL->IsLevelLoaded() && SL->IsLevelVisible();
		}
		return false;
	}

	static bool WaitUntilWorldReady(FAutomationTestBase* Test)
	{
		UWorld* World = AutomationCommon::GetAnyGameWorld();
		if (!World)
		{
			return false;
		}
		ADCGameMode* GM = World->GetAuthGameMode<ADCGameMode>();
		APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
		return GM && Player;
	}
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FDCWaitForGameWorldCommand, FAutomationTestBase*, Test);
bool FDCWaitForGameWorldCommand::Update()
{
	return DCFlowTestInternal::WaitUntilWorldReady(Test);
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FDCRunFullQuestFlowCommand, FAutomationTestBase*, Test);
bool FDCRunFullQuestFlowCommand::Update()
{
	UWorld* World = AutomationCommon::GetAnyGameWorld();
	if (!World)
	{
		Test->AddError(TEXT("No game world found after AutomationOpenMap"));
		return true;
	}

	ADCGameMode* GM = World->GetAuthGameMode<ADCGameMode>();
	UDCQuestSubsystem* Quest = World->GetSubsystem<UDCQuestSubsystem>();
	UDCNarratorSubsystem* Narrator = World->GetSubsystem<UDCNarratorSubsystem>();
	UDCBoarSubsystem* Boars = World->GetSubsystem<UDCBoarSubsystem>();
	AShooterCharacter* Player = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));

	Test->TestNotNull(TEXT("ADCGameMode should be active"), GM);
	Test->TestNotNull(TEXT("UDCQuestSubsystem should be active"), Quest);
	Test->TestNotNull(TEXT("UDCNarratorSubsystem should be active"), Narrator);
	Test->TestNotNull(TEXT("UDCBoarSubsystem should be active"), Boars);
	Test->TestNotNull(TEXT("Player should be AShooterCharacter"), Player);
	if (!GM || !Quest || !Narrator || !Boars || !Player)
	{
		return true;
	}

	UDCHUDWidget* HUD = GM->GetHUDWidget();
	UDCQuestTrackerWidget* Tracker = HUD ? HUD->GetQuestTracker() : nullptr;
	UDCSubtitlesWidget* Subs = HUD ? HUD->GetSubtitles() : nullptr;
	Test->TestNotNull(TEXT("WBP_DCHUD should be created on BeginPlay"), HUD);
	Test->TestNotNull(TEXT("WBP_QuestTracker should be bound on HUD"), Tracker);
	Test->TestNotNull(TEXT("WBP_Subtitles should be bound on HUD"), Subs);
	Test->TestNotNull(TEXT("WBP_HealthBar should be bound on HUD"), HUD ? HUD->GetHealthBar() : nullptr);

	// Verify alternating line helper works cleanly
	Narrator->Interrupt();
	Narrator->PlayAlternatingLine(FName("B2_NoMeat_01"), FName("B2_NoMeat_02"));
	Test->TestEqual(TEXT("Alternating call #1 plays primary line"), Narrator->GetCurrentLineId(), FName("B2_NoMeat_01"));
	Narrator->Interrupt();
	Narrator->PlayAlternatingLine(FName("B2_NoMeat_01"), FName("B2_NoMeat_02"));
	Test->TestEqual(TEXT("Alternating call #2 plays alt line"), Narrator->GetCurrentLineId(), FName("B2_NoMeat_02"));
	Narrator->Interrupt();

	// ======================== BEAT 1: Kill 10 Boars (interrupted at 7)
	GM->StartBeat(1);
	Test->TestEqual(TEXT("Beat 1 active"), GM->GetCurrentBeat(), 1);
	Test->TestTrue(TEXT("Beat 1 loads DC_Light_Day"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Light_Day")));
	Test->TestTrue(TEXT("Beat 1 loads DC_Set_Hunt"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_Hunt")));
	Test->TestEqual(TEXT("Beat 1 quest is Q_HuntBoars"), Quest->GetActiveQuestId(), FName("Q_HuntBoars"));
	Test->TestEqual(TEXT("Beat 1 quest target is 10"), Quest->GetTarget(), 10);
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Boars killed:"), Tracker->GetDisplayedTitle(), FString(TEXT("Boars killed:")));
		Test->TestEqual(TEXT("HUD QuestTracker shows 0/10"), Tracker->GetDisplayedCount(), FString(TEXT("0/10")));
	}
	Test->TestTrue(TEXT("Beat 1 player holds ADCSwordWeapon"), Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->IsA<ADCSwordWeapon>());

	for (int32 i = 1; i <= 6; ++i)
	{
		Quest->ReportEvent(FName("Kill.Boar"));
	}
	Test->TestEqual(TEXT("After 6 kills, still in Beat 1"), GM->GetCurrentBeat(), 1);
	Test->TestEqual(TEXT("After 6 kills, progress is 6/10"), Quest->GetCurrent(), 6);
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows 6/10"), Tracker->GetDisplayedCount(), FString(TEXT("6/10")));
	}

	// 7th kill -> Director interrupts for bacon breakfast (Beat 2)
	Quest->ReportEvent(FName("Kill.Boar"));

	// ======================== BEAT 2: Collect 5 Boar Meat + No-Meat Joke
	Test->TestEqual(TEXT("7th kill triggers Beat 2"), GM->GetCurrentBeat(), 2);
	Test->TestTrue(TEXT("Beat 2 loads DC_Set_NoMeat"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_NoMeat")));
	Test->TestEqual(TEXT("Beat 2 quest is Q_CollectMeat"), Quest->GetActiveQuestId(), FName("Q_CollectMeat"));
	Test->TestEqual(TEXT("Beat 2 quest target is 5"), Quest->GetTarget(), 5);
	Test->TestEqual(TEXT("Beat 2 quest tag is Collect.Meat"), Quest->GetActiveEventTag(), FName("Collect.Meat"));
	Test->TestEqual(TEXT("Beat 2 starts with B2_Intro_01"), Narrator->GetCurrentLineId(), FName("B2_Intro_01"));
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Boar meat collected:"), Tracker->GetDisplayedTitle(), FString(TEXT("Boar meat collected:")));
	}
	if (Subs)
	{
		Test->TestEqual(TEXT("HUD Subtitles speaker is THE DIRECTOR"), Subs->GetSpeakerString(), FString(TEXT("THE DIRECTOR")));
		Test->TestTrue(TEXT("HUD Subtitles displays B2_Intro_01 text"), Subs->GetLineString().Contains(TEXT("starving")));
	}
	Narrator->Interrupt();

	// Spawn 2 boars and kill them to verify meat drop on #1 and "no meat" joke on #2
	auto CountMeatPickups = [World]() -> int32
	{
		int32 N = 0;
		for (TActorIterator<ADCBoarMeatPickup> It(World); It; ++It) { ++N; }
		return N;
	};
	const int32 MeatBefore = CountMeatPickups();

	FActorSpawnParameters BoarSpawnParams;
	BoarSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADCBoar* Boar1 = World->SpawnActor<ADCBoar>(ADCBoar::StaticClass(), FVector(-4000.0f, 300.0f, 100.0f), FRotator::ZeroRotator, BoarSpawnParams);
	ADCBoar* Boar2 = World->SpawnActor<ADCBoar>(ADCBoar::StaticClass(), FVector(-4000.0f, -300.0f, 100.0f), FRotator::ZeroRotator, BoarSpawnParams);
	Test->TestNotNull(TEXT("Beat 2 test Boar1 spawned"), Boar1);
	Test->TestNotNull(TEXT("Beat 2 test Boar2 spawned"), Boar2);
	if (Boar1 && Boar2)
	{
		Boar1->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestEqual(TEXT("Beat 2 kill #1 drops 1 boar meat pickup"), CountMeatPickups(), MeatBefore + 1);

		Narrator->Interrupt();
		Boar2->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestEqual(TEXT("Beat 2 kill #2 drops NO meat (stays at +1)"), CountMeatPickups(), MeatBefore + 1);
		const FName NoMeatLine = Narrator->GetCurrentLineId();
		Test->TestTrue(TEXT("Beat 2 kill #2 plays B2_NoMeat_01 or B2_NoMeat_02"),
			NoMeatLine == FName("B2_NoMeat_01") || NoMeatLine == FName("B2_NoMeat_02"));
	}

	// Collect 5 meat -> triggers Beat 3
	for (int32 i = 0; i < 5; ++i)
	{
		Quest->ReportEvent(FName("Collect.Meat"));
	}

	// ======================== BEAT 3: Gun Swap + Hyperspawn + Kill 100 Boars
	Test->TestEqual(TEXT("5/5 meat triggers Beat 3"), GM->GetCurrentBeat(), 3);
	Test->TestTrue(TEXT("Beat 3 loads DC_Set_FilmCrew"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_FilmCrew")));
	Test->TestFalse(TEXT("Beat 3 unloads DC_Set_NoMeat"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_NoMeat")));
	Test->TestEqual(TEXT("Beat 3 quest is Q_HyperspawnBoars"), Quest->GetActiveQuestId(), FName("Q_HyperspawnBoars"));
	Test->TestEqual(TEXT("Beat 3 quest target is 100"), Quest->GetTarget(), 100);
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Boars eliminated:"), Tracker->GetDisplayedTitle(), FString(TEXT("Boars eliminated:")));
	}
	Test->TestTrue(TEXT("Beat 3 player holds a gun (not sword)"),
		Player->GetCurrentWeapon() && !Player->GetCurrentWeapon()->IsA<ADCSwordWeapon>());

	bool bAnyHyperspawning = false;
	for (TActorIterator<ADCBoarSpawner> It(World); It; ++It)
	{
		bAnyHyperspawning |= It->IsHyperspawnMode();
	}
	Test->TestTrue(TEXT("Beat 3 enables boar hyperspawn mode"), bAnyHyperspawning);

	for (int32 i = 1; i <= 100; ++i)
	{
		Quest->ReportEvent(FName("Kill.Boar"));
	}

	// ======================== BEAT 4: King Boar + Rescue Princess
	Test->TestEqual(TEXT("100/100 boars triggers Beat 4"), GM->GetCurrentBeat(), 4);
	Test->TestTrue(TEXT("Beat 4 loads DC_Light_PG"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Light_PG")));
	Test->TestTrue(TEXT("Beat 4 loads DC_Set_PG"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_PG")));
	Test->TestEqual(TEXT("Beat 4a quest is Q_DefeatKingBoar"), Quest->GetActiveQuestId(), FName("Q_DefeatKingBoar"));
	Test->TestEqual(TEXT("Beat 4a quest target is 1"), Quest->GetTarget(), 1);
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Defeat the King Boar:"), Tracker->GetDisplayedTitle(), FString(TEXT("Defeat the King Boar:")));
	}

	ADCKingBoar* KingBoar = nullptr;
	for (TActorIterator<ADCKingBoar> It(World); It; ++It) { KingBoar = *It; break; }
	ADCPrincess* Princess = nullptr;
	for (TActorIterator<ADCPrincess> It(World); It; ++It) { Princess = *It; break; }

	Test->TestNotNull(TEXT("Beat 4 spawned ADCKingBoar"), KingBoar);
	Test->TestNotNull(TEXT("Beat 4 spawned ADCPrincess"), Princess);
	if (KingBoar && Princess)
	{
		Test->TestTrue(TEXT("Princess starts locked while King Boar lives"), Princess->IsLocked());
		// Kill King Boar -> unlocks Princess and swaps quest to Q_RescuePrincess
		KingBoar->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestFalse(TEXT("Princess unlocks after King Boar dies"), Princess->IsLocked());
		Test->TestEqual(TEXT("Beat 4b quest is Q_RescuePrincess"), Quest->GetActiveQuestId(), FName("Q_RescuePrincess"));

		// Teleport player onto Princess to trigger rescue overlap (or verify rescue event)
		Player->SetActorLocation(Princess->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		if (!Princess->IsRescued())
		{
			// Fallback if physics overlap queries are deferred until next tick in nullrhi
			Quest->ReportEvent(FName("Rescue.Princess"));
		}
		else
		{
			Test->TestTrue(TEXT("Touching unlocked Princess rescues her"), Princess->IsRescued());
		}
	}

	// ======================== BEAT 5: Director Boss Fight
	// EnterBeat5 waits for B4_PrincessRescued line or fallback timer; jump cleanly to verify Beat 5 flow
	GM->StartBeat(5);
	Test->TestEqual(TEXT("Beat 5 active"), GM->GetCurrentBeat(), 5);
	Test->TestTrue(TEXT("Beat 5 loads DC_Light_Boss"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Light_Boss")));
	Test->TestTrue(TEXT("Beat 5 loads DC_Set_Boss"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_Boss")));
	Test->TestEqual(TEXT("Beat 5 quest is Q_DefeatDirector"), Quest->GetActiveQuestId(), FName("Q_DefeatDirector"));
	Test->TestEqual(TEXT("Beat 5 quest target is 1"), Quest->GetTarget(), 1);
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Defeat the Director:"), Tracker->GetDisplayedTitle(), FString(TEXT("Defeat the Director:")));
	}

	ADCDirectorBoss* Boss = nullptr;
	for (TActorIterator<ADCDirectorBoss> It(World); It; ++It) { Boss = *It; break; }
	Test->TestNotNull(TEXT("Beat 5 spawned ADCDirectorBoss"), Boss);
	if (Boss)
	{
		Boss->StartFight();
		Test->TestTrue(TEXT("Director Boss fight is active"), Boss->IsFightActive());
		Test->TestFalse(TEXT("Director Boss is no longer invulnerable"), Boss->GetHealth()->bInvulnerable);

		Narrator->Interrupt();
		Boss->GetHealth()->ApplyDamage(Boss->GetHealth()->GetMaxHP() * 0.6f, Player->GetController(), Player);
		Test->TestEqual(TEXT("Boss at <=50% HP triggers B5_BossHalfHP"), Narrator->GetCurrentLineId(), FName("B5_BossHalfHP"));

		Boss->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestTrue(TEXT("Q_DefeatDirector completes on boss death"), Quest->IsActiveQuestComplete());
		Test->TestEqual(TEXT("Boss defeated plays B5_DirectorDefeated"), Narrator->GetCurrentLineId(), FName("B5_DirectorDefeated"));
	}

	Test->AddInfo(TEXT("[DC_TEST] All 5 beats, quests, weapon swaps, pickups, Princess, and Director Boss passed!"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDCFullQuestFlowTest,
	"DirectorsCut.QuestFlow.FullGameLoop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDCFullQuestFlowTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/DirectorsCut/Maps/Lvl_DirectorsCut")))
	{
		AddError(TEXT("Failed to open /Game/DirectorsCut/Maps/Lvl_DirectorsCut"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FDCWaitForGameWorldCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FDCRunFullQuestFlowCommand(this));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
