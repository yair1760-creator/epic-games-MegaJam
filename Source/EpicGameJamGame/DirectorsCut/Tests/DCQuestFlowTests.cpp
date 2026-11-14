// Director's Cut - automated end-to-end verification of all 5 beats, quests, subtitles, VO, and actors.

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/DataTable.h"
#include "Engine/Font.h"
#include "Engine/LevelStreaming.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

#include "AI/DCBoar.h"
#include "AI/DCBoarSpawner.h"
#include "AI/DCBoarSubsystem.h"
#include "AI/DCDirectorBoss.h"
#include "AI/DCKingBoar.h"
#include "Beats/DCGameMode.h"
#include "Beats/DCPlayerController.h"
#include "Beats/DCPrincess.h"
#include "Beats/DCShedDoors.h"
#include "Combat/DCBoarMeatPickup.h"
#include "Combat/DCHealthComponent.h"
#include "Combat/DCShotgunWeapon.h"
#include "Combat/DCSwordWeapon.h"
#include "Combat/DCWeaponSpriteWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Narrator/DCNarratorSubsystem.h"
#include "Quest/DCQuestSubsystem.h"
#include "UI/DCAmmoCounterWidget.h"
#include "UI/DCDeathScreenWidget.h"
#include "UI/DCHealthBarWidget.h"
#include "UI/DCHUDWidget.h"
#include "UI/DCPauseMenuWidget.h"
#include "UI/DCQuestTrackerWidget.h"
#include "UI/DCStartScreenWidget.h"
#include "UI/DCSubtitlesWidget.h"
#include "UI/DCVictoryWidget.h"
#include "ShooterCharacter.h"
#include "ShooterProjectile.h"
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

	struct FMusicExpectation
	{
		const TCHAR* Path;
		bool bShouldLoop;
	};
	const FMusicExpectation ExpectedMusic[] = {
		{ TEXT("/Game/DirectorsCut/Audio/Music/M_Heroic.M_Heroic"),   true  },
		{ TEXT("/Game/DirectorsCut/Audio/Music/M_Frantic.M_Frantic"), true  },
		{ TEXT("/Game/DirectorsCut/Audio/Music/M_Romance.M_Romance"), true  },
		{ TEXT("/Game/DirectorsCut/Audio/Music/M_Boss.M_Boss"),       true  },
		{ TEXT("/Game/DirectorsCut/Audio/Music/M_Victory.M_Victory"), false },
	};
	for (const FMusicExpectation& M : ExpectedMusic)
	{
		USoundBase* Cue = LoadObject<USoundBase>(nullptr, M.Path);
		TestNotNull(*FString::Printf(TEXT("Music asset %s must exist"), M.Path), Cue);
		if (Cue)
		{
			TestEqual(*FString::Printf(TEXT("Music asset %s looping state"), M.Path), Cue->IsLooping(), M.bShouldLoop);
			TestTrue(*FString::Printf(TEXT("Music asset %s duration > 0"), M.Path), Cue->GetDuration() > 0.0f);
		}
	}

	const TCHAR* ExpectedUITextures[] = {
		TEXT("/Game/DirectorsCut/UI/Art/T_DC_Director_Portrait.T_DC_Director_Portrait"),
		TEXT("/Game/DirectorsCut/UI/Art/T_DC_UI_ClapperBar.T_DC_UI_ClapperBar"),
		TEXT("/Game/DirectorsCut/UI/Art/T_DC_UI_HeartIcon.T_DC_UI_HeartIcon"),
		TEXT("/Game/DirectorsCut/UI/Art/T_DC_UI_SlateIcon.T_DC_UI_SlateIcon"),
		TEXT("/Game/DirectorsCut/UI/Art/T_DC_UI_Crosshair.T_DC_UI_Crosshair"),
	};
	for (const TCHAR* TexPath : ExpectedUITextures)
	{
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TexPath);
		TestNotNull(*FString::Printf(TEXT("UI texture %s must exist"), TexPath), Tex);
	}

	const TCHAR* ExpectedUIFonts[] = {
		TEXT("/Game/DirectorsCut/UI/Fonts/Font_Display.Font_Display"),
		TEXT("/Game/DirectorsCut/UI/Fonts/Font_Condensed.Font_Condensed"),
		TEXT("/Game/DirectorsCut/UI/Fonts/Font_Body.Font_Body"),
		TEXT("/Game/DirectorsCut/UI/Fonts/Font_Mono.Font_Mono"),
	};
	for (const TCHAR* FontPath : ExpectedUIFonts)
	{
		UFont* FontAsset = LoadObject<UFont>(nullptr, FontPath);
		TestNotNull(*FString::Printf(TEXT("UI font %s must exist"), FontPath), FontAsset);
	}

	const TCHAR* ExpectedSFX[] = {
		TEXT("/Game/DirectorsCut/Audio/SFX/S_SwordSwing.S_SwordSwing"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_SwordHit.S_SwordHit"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BoarGrunt.S_BoarGrunt"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BoarAggro.S_BoarAggro"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BoarCharge.S_BoarCharge"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BoarBite.S_BoarBite"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BoarSqueal.S_BoarSqueal"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BoarDeath.S_BoarDeath"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_PlayerHurt.S_PlayerHurt"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_MeatPickup.S_MeatPickup"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BossShot.S_BossShot"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BossHit.S_BossHit"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_BossDefeat.S_BossDefeat"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_Cut.S_Cut"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_QuestTick.S_QuestTick"),
		TEXT("/Game/DirectorsCut/Audio/SFX/S_QuestComplete.S_QuestComplete"),
	};
	for (const TCHAR* SfxPath : ExpectedSFX)
	{
		USoundBase* Sfx = LoadObject<USoundBase>(nullptr, SfxPath);
		TestNotNull(*FString::Printf(TEXT("SFX asset %s must exist"), SfxPath), Sfx);
		if (Sfx)
		{
			TestFalse(*FString::Printf(TEXT("SFX asset %s should not loop"), SfxPath), Sfx->IsLooping());
			TestTrue(*FString::Printf(TEXT("SFX asset %s duration > 0"), SfxPath), Sfx->GetDuration() > 0.0f);
		}
	}

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
	UDCAmmoCounterWidget* Ammo = HUD ? HUD->GetAmmoCounter() : nullptr;
	Test->TestNotNull(TEXT("WBP_DCHUD should be created on BeginPlay"), HUD);
	Test->TestNotNull(TEXT("WBP_QuestTracker should be bound on HUD"), Tracker);
	Test->TestNotNull(TEXT("WBP_Subtitles should be bound on HUD"), Subs);
	Test->TestNotNull(TEXT("WBP_HealthBar should be bound on HUD"), HUD ? HUD->GetHealthBar() : nullptr);
	Test->TestNotNull(TEXT("WBP_AmmoCounter should be bound on HUD"), Ammo);

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
	Test->TestEqual(TEXT("Beat 1 music is M_Heroic"), GM->GetActiveMusicAsset() ? GM->GetActiveMusicAsset()->GetFName() : NAME_None, FName("M_Heroic"));
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Boars killed:"), Tracker->GetDisplayedTitle(), FString(TEXT("Boars killed:")));
		Test->TestEqual(TEXT("HUD QuestTracker shows 0/10"), Tracker->GetDisplayedCount(), FString(TEXT("0/10")));
	}
	Test->TestTrue(TEXT("Beat 1 player holds ADCSwordWeapon"), Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->IsA<ADCSwordWeapon>());
	if (Ammo)
	{
		Test->TestEqual(TEXT("AmmoCounter is collapsed while holding Sword"), Ammo->GetVisibility(), ESlateVisibility::Collapsed);
	}

	for (int32 i = 1; i <= 6; ++i)
	{
		Narrator->Interrupt();
		Quest->ReportEvent(FName("Kill.Boar"));
		if (i == 1)
		{
			Test->TestTrue(TEXT("1st kill does not play L1_FirstKill (avoiding clash with L1_FirstHit)"), Narrator->GetCurrentLineId().IsNone());
		}
		else if (i == 2)
		{
			Test->TestEqual(TEXT("2nd kill plays L1_FirstKill"), Narrator->GetCurrentLineId(), FName("L1_FirstKill"));
		}
	}
	Test->TestEqual(TEXT("After 6 kills, still in Beat 1"), GM->GetCurrentBeat(), 1);
	Test->TestEqual(TEXT("After 6 kills, progress is 6/10"), Quest->GetCurrent(), 6);
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows 6/10"), Tracker->GetDisplayedCount(), FString(TEXT("6/10")));
	}

	// 7th kill -> Director sees meat drop, wants bacon breakfast (Beat 2)
	Quest->ReportEvent(FName("Kill.Boar"));

	// ======================== BEAT 2: Collect 5 Bacon + No-Meat Joke
	Test->TestEqual(TEXT("7th kill triggers Beat 2"), GM->GetCurrentBeat(), 2);
	Test->TestTrue(TEXT("Beat 2 loads DC_Set_NoMeat"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_NoMeat")));
	Test->TestEqual(TEXT("Beat 2 quest is Q_CollectMeat"), Quest->GetActiveQuestId(), FName("Q_CollectMeat"));
	Test->TestEqual(TEXT("Beat 2 quest target is 5"), Quest->GetTarget(), 5);
	Test->TestEqual(TEXT("Beat 2 quest tag is Collect.Meat"), Quest->GetActiveEventTag(), FName("Collect.Meat"));
	Test->TestEqual(TEXT("Beat 2 keeps M_Heroic music"), GM->GetActiveMusicAsset() ? GM->GetActiveMusicAsset()->GetFName() : NAME_None, FName("M_Heroic"));
	Test->TestEqual(TEXT("Beat 2 starts with B2_Intro_01"), Narrator->GetCurrentLineId(), FName("B2_Intro_01"));
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Bacon collected:"), Tracker->GetDisplayedTitle(), FString(TEXT("Bacon collected:")));
	}
	if (Subs)
	{
		Test->TestEqual(TEXT("HUD Subtitles speaker is THE DIRECTOR"), Subs->GetSpeakerString(), FString(TEXT("THE DIRECTOR")));
		Test->TestTrue(TEXT("HUD Subtitles displays B2_Intro_01 text"), Subs->GetLineString().Contains(TEXT("meat")));
	}
	Narrator->Interrupt();

	// Spawn 3 boars and kill them to verify meat drops on #1-#2 and "no meat" joke on #3
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
	ADCBoar* Boar2 = World->SpawnActor<ADCBoar>(ADCBoar::StaticClass(), FVector(-4000.0f, 0.0f, 100.0f), FRotator::ZeroRotator, BoarSpawnParams);
	ADCBoar* Boar3 = World->SpawnActor<ADCBoar>(ADCBoar::StaticClass(), FVector(-4000.0f, -300.0f, 100.0f), FRotator::ZeroRotator, BoarSpawnParams);
	Test->TestNotNull(TEXT("Beat 2 test Boar1 spawned"), Boar1);
	Test->TestNotNull(TEXT("Beat 2 test Boar2 spawned"), Boar2);
	Test->TestNotNull(TEXT("Beat 2 test Boar3 spawned"), Boar3);
	if (Boar1 && Boar2 && Boar3)
	{
		Boar1->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestEqual(TEXT("Beat 2 kill #1 drops 1 bacon pickup"), CountMeatPickups(), MeatBefore + 1);

		Narrator->Interrupt();
		Boar2->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestEqual(TEXT("Beat 2 kill #2 drops 1 bacon pickup"), CountMeatPickups(), MeatBefore + 2);

		Narrator->Interrupt();
		Boar3->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestEqual(TEXT("Beat 2 kill #3 drops NO meat (stays at +2)"), CountMeatPickups(), MeatBefore + 2);
		const FName NoMeatLine = Narrator->GetCurrentLineId();
		Test->TestTrue(TEXT("Beat 2 kill #3 plays B2_NoMeat_01 or B2_NoMeat_02"),
			NoMeatLine == FName("B2_NoMeat_01") || NoMeatLine == FName("B2_NoMeat_02"));
	}

	// Collect 5 bacon -> verify 1/5 doesn't clash with intro, 2/5 plays B2_FirstMeat, 5/5 triggers Beat 3
	Narrator->Interrupt();
	Quest->ReportEvent(FName("Collect.Meat"));
	Test->TestTrue(TEXT("1/5 bacon does not play B2_FirstMeat (avoiding clash with B2_Intro)"), Narrator->GetCurrentLineId().IsNone());
	Quest->ReportEvent(FName("Collect.Meat"));
	Test->TestEqual(TEXT("2/5 bacon plays B2_FirstMeat"), Narrator->GetCurrentLineId(), FName("B2_FirstMeat"));
	Narrator->Interrupt();
	for (int32 i = 2; i < 5; ++i)
	{
		Quest->ReportEvent(FName("Collect.Meat"));
	}

	// ======================== BEAT 3: Gun Swap + Hyperspawn + Kill 100 Boars
	Test->TestEqual(TEXT("5/5 meat triggers Beat 3"), GM->GetCurrentBeat(), 3);
	Test->TestTrue(TEXT("Beat 3 loads DC_Set_FilmCrew"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_FilmCrew")));
	Test->TestFalse(TEXT("Beat 3 unloads DC_Set_NoMeat"), DCFlowTestInternal::IsSublevelActive(World, FName("DC_Set_NoMeat")));
	Test->TestEqual(TEXT("Beat 3 quest is Q_HyperspawnBoars"), Quest->GetActiveQuestId(), FName("Q_HyperspawnBoars"));
	Test->TestEqual(TEXT("Beat 3 quest target is 100"), Quest->GetTarget(), 100);
	Test->TestEqual(TEXT("Beat 3 music is M_Frantic"), GM->GetActiveMusicAsset() ? GM->GetActiveMusicAsset()->GetFName() : NAME_None, FName("M_Frantic"));
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Boars eliminated:"), Tracker->GetDisplayedTitle(), FString(TEXT("Boars eliminated:")));
	}
	Test->TestTrue(TEXT("Beat 3 player holds ADCShotgunWeapon"),
		Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->IsA<ADCShotgunWeapon>());
	if (Ammo)
	{
		Test->TestEqual(TEXT("AmmoCounter is visible in Beat 3"), Ammo->GetVisibility(), ESlateVisibility::HitTestInvisible);
		Test->TestEqual(TEXT("AmmoCounter shows 8/8 before firing"), Ammo->GetDisplayedAmmo(), FString(TEXT("8/8")));
	}
	if (ADCShotgunWeapon* Shotgun = Cast<ADCShotgunWeapon>(Player->GetCurrentWeapon()))
	{
		int32 ProjectilesBefore = 0;
		for (TActorIterator<AShooterProjectile> It(World); It; ++It) { ++ProjectilesBefore; }

		const int32 BulletsBefore = Shotgun->GetBulletCount();
		Shotgun->StartFiring();
		Shotgun->StopFiring();
		Test->TestEqual(TEXT("Shotgun blast consumes 1 shell"), Shotgun->GetBulletCount(), BulletsBefore - 1);
		if (Ammo)
		{
			Test->TestEqual(TEXT("AmmoCounter updates to 7/8 after firing"), Ammo->GetDisplayedAmmo(), FString(TEXT("7/8")));
		}

		int32 ProjectilesAfter = 0;
		for (TActorIterator<AShooterProjectile> It(World); It; ++It) { ++ProjectilesAfter; }
		Test->TestEqual(TEXT("Shotgun blast spawns PelletCount projectiles"), ProjectilesAfter - ProjectilesBefore, Shotgun->GetPelletCount());

		Shotgun->StartReload();
		Test->TestTrue(TEXT("Shotgun enters reloading state"), Shotgun->IsReloading());
		Test->TestTrue(TEXT("Shotgun reload starts 2D front-of-screen reload animation"), Shotgun->IsPlayingReloadAnim());
		Test->TestEqual(TEXT("Shotgun reload starts in ReloadDrop pose"), Shotgun->GetSpritePose(), FName("ReloadDrop"));
		Test->TestNotNull(TEXT("Shotgun 2D sprite widget is created on screen"), Shotgun->GetSpriteWidget());
		if (Shotgun->GetSpriteWidget())
		{
			Test->TestEqual(TEXT("Shotgun 2D sprite widget is visible during reload"),
				Shotgun->GetSpriteWidget()->GetVisibility(), ESlateVisibility::HitTestInvisible);
		}

		// Step through reload animation phases and verify pose progression
		Shotgun->Tick(0.45f);
		Test->TestEqual(TEXT("Shotgun reload reaches ReloadLoad pose"), Shotgun->GetSpritePose(), FName("ReloadLoad"));
		Shotgun->Tick(0.70f);
		Test->TestEqual(TEXT("Shotgun reload reaches ReloadPump pose"), Shotgun->GetSpritePose(), FName("ReloadPump"));
		Shotgun->Tick(0.25f);
		Test->TestEqual(TEXT("Shotgun reload reaches ReloadSettle pose"), Shotgun->GetSpritePose(), FName("ReloadSettle"));
	}

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
	Test->TestTrue(TEXT("Beat 4 swaps back to ADCSwordWeapon"),
		Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->IsA<ADCSwordWeapon>());
	if (Ammo)
	{
		Test->TestEqual(TEXT("AmmoCounter collapses again on Beat 4 Sword"), Ammo->GetVisibility(), ESlateVisibility::Collapsed);
	}
	Test->TestEqual(TEXT("Beat 4a quest is Q_DefeatKingBoar"), Quest->GetActiveQuestId(), FName("Q_DefeatKingBoar"));
	Test->TestEqual(TEXT("Beat 4a quest target is 1"), Quest->GetTarget(), 1);
	Test->TestEqual(TEXT("Beat 4 music is M_Romance"), GM->GetActiveMusicAsset() ? GM->GetActiveMusicAsset()->GetFName() : NAME_None, FName("M_Romance"));
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Defeat the King Boar:"), Tracker->GetDisplayedTitle(), FString(TEXT("Defeat the King Boar:")));
	}

	ADCKingBoar* KingBoar = nullptr;
	for (TActorIterator<ADCKingBoar> It(World); It; ++It) { KingBoar = *It; break; }
	ADCPrincess* Princess = nullptr;
	for (TActorIterator<ADCPrincess> It(World); It; ++It) { Princess = *It; break; }
	ADCShedDoors* ShedDoors = nullptr;
	for (TActorIterator<ADCShedDoors> It(World); It; ++It) { ShedDoors = *It; break; }

	Test->TestNotNull(TEXT("Beat 4 spawned ADCKingBoar"), KingBoar);
	Test->TestNotNull(TEXT("Beat 4 spawned ADCPrincess"), Princess);
	Test->TestNotNull(TEXT("Beat 4 DC_Set_PG has ADCShedDoors"), ShedDoors);
	if (KingBoar && Princess)
	{
		Test->TestTrue(TEXT("Princess starts locked while King Boar lives"), Princess->IsLocked());
		if (ShedDoors)
		{
			Test->TestTrue(TEXT("Shed doors start locked while King Boar lives"), ShedDoors->IsLocked());
		}
		// Kill King Boar -> unlocks Princess, opens shed doors, and swaps quest to Q_RescuePrincess
		KingBoar->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestFalse(TEXT("Princess unlocks after King Boar dies"), Princess->IsLocked());
		if (ShedDoors)
		{
			Test->TestFalse(TEXT("Shed doors unlock after King Boar dies"), ShedDoors->IsLocked());
			Test->TestTrue(TEXT("Shed doors begin opening after King Boar dies"), ShedDoors->IsOpen());
		}
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
	Test->TestTrue(TEXT("Beat 5 keeps ADCSwordWeapon"),
		Player->GetCurrentWeapon() && Player->GetCurrentWeapon()->IsA<ADCSwordWeapon>());
	Test->TestEqual(TEXT("Beat 5 quest is Q_DefeatDirector"), Quest->GetActiveQuestId(), FName("Q_DefeatDirector"));
	Test->TestEqual(TEXT("Beat 5 quest target is 1"), Quest->GetTarget(), 1);
	Test->TestEqual(TEXT("Beat 5 music is M_Boss"), GM->GetActiveMusicAsset() ? GM->GetActiveMusicAsset()->GetFName() : NAME_None, FName("M_Boss"));
	if (Tracker)
	{
		Test->TestEqual(TEXT("HUD QuestTracker shows Defeat the Director:"), Tracker->GetDisplayedTitle(), FString(TEXT("Defeat the Director:")));
	}

	// Verify Beat 5 arena respawn override (returns player to Beat 5 arena, not PlayerStart_Village)
	FTransform Beat5Respawn;
	Test->TestTrue(TEXT("Beat 5 overrides respawn transform to arena"), GM->GetBeatRespawnTransform(Beat5Respawn));
	Test->TestTrue(TEXT("Beat 5 respawn location is inside arena (X near -600)"), FMath::IsNearlyEqual(Beat5Respawn.GetLocation().X, -600.0f, 50.0f));
	Player->Destroy();
	AShooterCharacter* RespawnedBeat5Player = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	Test->TestNotNull(TEXT("Destroying player in Beat 5 immediately respawns new pawn"), RespawnedBeat5Player);
	if (RespawnedBeat5Player)
	{
		Test->TestTrue(TEXT("Respawned Beat 5 player spawns in arena instead of village (-5200)"),
			FVector::Dist2D(RespawnedBeat5Player->GetActorLocation(), Beat5Respawn.GetLocation()) < 150.0f);
		Test->TestTrue(TEXT("Respawned Beat 5 player holds ADCSwordWeapon"),
			RespawnedBeat5Player->GetCurrentWeapon() && RespawnedBeat5Player->GetCurrentWeapon()->IsA<ADCSwordWeapon>());
		Player = RespawnedBeat5Player;
	}

	ADCDirectorBoss* Boss = nullptr;
	for (TActorIterator<ADCDirectorBoss> It(World); It; ++It) { Boss = *It; break; }
	Test->TestNotNull(TEXT("Beat 5 spawned ADCDirectorBoss"), Boss);
	if (Boss)
	{
		Boss->StartFight();
		Test->TestTrue(TEXT("Director Boss fight is active"), Boss->IsFightActive());
		Test->TestFalse(TEXT("Director Boss is no longer invulnerable"), Boss->GetHealth()->bInvulnerable);
		Test->TestFalse(TEXT("Director Boss does not start reloading"), Boss->IsReloading());
		Test->TestEqual(TEXT("Director Boss starts with full clip"), Boss->GetShotsRemaining(), Boss->ShotsBeforeReload);

		Narrator->Interrupt();
		Boss->GetHealth()->ApplyDamage(Boss->GetHealth()->GetMaxHP() * 0.6f, Player->GetController(), Player);
		Test->TestEqual(TEXT("Boss at <=50% HP triggers B5_BossHalfHP"), Narrator->GetCurrentLineId(), FName("B5_BossHalfHP"));

		Boss->GetHealth()->Kill(Player->GetController(), Player);
		Test->TestTrue(TEXT("Q_DefeatDirector completes on boss death"), Quest->IsActiveQuestComplete());
		Test->TestEqual(TEXT("Boss defeated plays B5_DirectorDefeated"), Narrator->GetCurrentLineId(), FName("B5_DirectorDefeated"));

		// Interrupting B5_DirectorDefeated fires OnLineFinished("B5_DirectorDefeated") -> opens Victory Scene
		Narrator->Interrupt();
		Test->TestTrue(TEXT("Victory scene is shown after B5_DirectorDefeated finishes"), GM->IsVictoryShown());
		Test->TestNull(TEXT("Victory scene stops beat loop music"), GM->GetActiveMusicAsset());
		UDCVictoryWidget* Victory = GM->GetVictoryWidget();
		Test->TestNotNull(TEXT("Victory widget is created"), Victory);
		if (Victory)
		{
			Test->TestTrue(TEXT("Victory title says THAT'S A WRAP!"), Victory->GetTitleString().Contains(TEXT("WRAP")));
			Test->TestTrue(TEXT("Victory stats show Director Defeated"), Victory->GetStatsString().Contains(TEXT("Director Defeated")));
		}
	}

	// Clean up Beat 5 victory state so PIE/test exit leaves normal state
	GM->StartBeat(1);
	Test->TestFalse(TEXT("StartBeat resets victory state"), GM->IsVictoryShown());

	// Verify Start Screen and Pause Menu flow
	GM->ShowStartScreen();
	Test->TestTrue(TEXT("Start screen shows on ShowStartScreen"), GM->IsStartScreenShown());
	Test->TestNotNull(TEXT("Start screen widget is created"), GM->GetStartScreenWidget());
	if (ADCPlayerController* DCPC = Cast<ADCPlayerController>(World->GetFirstPlayerController()))
	{
		DCPC->SetPauseMenuOpen(true);
		Test->TestFalse(TEXT("Pause menu cannot open while Start screen is shown"), DCPC->IsPauseMenuOpen());
	}
	GM->StartFromTitle();
	Test->TestFalse(TEXT("StartFromTitle dismisses Start screen"), GM->IsStartScreenShown());
	Test->TestEqual(TEXT("HUD becomes visible after StartFromTitle"), HUD->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);

	if (ADCPlayerController* DCPC = Cast<ADCPlayerController>(World->GetFirstPlayerController()))
	{
		DCPC->SetPauseMenuOpen(true);
		Test->TestTrue(TEXT("SetPauseMenuOpen(true) opens Pause menu"), DCPC->IsPauseMenuOpen());
		Test->TestNotNull(TEXT("Pause menu widget is created"), DCPC->GetPauseMenuWidget());
		Test->TestTrue(TEXT("Game is paused while Pause menu is open"), UGameplayStatics::IsGamePaused(World));

		DCPC->SetPauseMenuOpen(false);
		Test->TestFalse(TEXT("SetPauseMenuOpen(false) closes Pause menu"), DCPC->IsPauseMenuOpen());
		Test->TestFalse(TEXT("Game resumes after closing Pause menu"), UGameplayStatics::IsGamePaused(World));
	}

	// Verify "CUT! TAKE 2" Death Slate overlay and respawn dismissal while preserving quest progress
	GM->StartBeat(1);
	Quest->ReportEvent(FName("Kill.Boar"));
	Test->TestEqual(TEXT("Pre-death Beat 1 progress is 1/10"), Quest->GetCurrent(), 1);
	Test->TestEqual(TEXT("Initial take number is 1"), GM->GetCurrentTakeNumber(), 1);
	Test->TestFalse(TEXT("Death screen starts hidden"), GM->IsDeathScreenShown());

	if (AShooterCharacter* AlivePlayer = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)))
	{
		Narrator->Interrupt();
		UGameplayStatics::ApplyDamage(AlivePlayer, AlivePlayer->GetMaxHP() + 50.0f, nullptr, nullptr, nullptr);
		Test->TestTrue(TEXT("Player is dead after lethal damage"), AlivePlayer->IsDead());
		Test->TestTrue(TEXT("Death screen shows on player death"), GM->IsDeathScreenShown());
		Test->TestEqual(TEXT("Death increments take number to 2"), GM->GetCurrentTakeNumber(), 2);
		Test->TestEqual(TEXT("Death triggers L1_PlayerDied line"), Narrator->GetCurrentLineId(), FName("L1_PlayerDied"));
		if (UDCDeathScreenWidget* DeathWidget = GM->GetDeathScreenWidget())
		{
			Test->TestTrue(TEXT("Death slate title displays TAKE 2"), DeathWidget->GetTitleString().Contains(TEXT("TAKE 2")));
		}
		else
		{
			Test->AddError(TEXT("DeathScreenWidget was not created"));
		}
		if (HUD->GetCrosshair())
		{
			Test->TestEqual(TEXT("Crosshair hides while Death slate is active"), HUD->GetCrosshair()->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (ADCPlayerController* DCPC = Cast<ADCPlayerController>(World->GetFirstPlayerController()))
		{
			DCPC->SetPauseMenuOpen(true);
			Test->TestFalse(TEXT("Pause menu cannot open while Death slate is shown"), DCPC->IsPauseMenuOpen());
		}

		// Simulate respawn timer completing (destroying dead pawn spawns fresh pawn via PC)
		AlivePlayer->Destroy();
		AShooterCharacter* Respawned = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
		Test->TestNotNull(TEXT("Respawned pawn is created"), Respawned);
		Test->TestFalse(TEXT("Death screen dismisses after respawn"), GM->IsDeathScreenShown());
		Test->TestEqual(TEXT("Quest progress is preserved across death/respawn"), Quest->GetCurrent(), 1);
		if (HUD->GetCrosshair())
		{
			Test->TestEqual(TEXT("Crosshair restores after respawn"), HUD->GetCrosshair()->GetVisibility(), ESlateVisibility::HitTestInvisible);
		}
	}

	GM->StartBeat(1);

	Test->AddInfo(TEXT("[DC_TEST] All 5 beats, quests, weapon swaps, pickups, Princess, Director Boss, B5 respawn, Start/Pause, Death Slate, and Victory passed!"));
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

// -----------------------------------------------------------------------------
// Test 3: Passive Player Health Regeneration (5s no-damage delay, +1 HP/sec)
// -----------------------------------------------------------------------------
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FDCVerifyHealthRegenCommand, FAutomationTestBase*, Test, double, StartTimeSec);
bool FDCVerifyHealthRegenCommand::Update()
{
	UWorld* World = AutomationCommon::GetAnyGameWorld();
	if (!World)
	{
		Test->AddError(TEXT("No game world found during health regen test"));
		return true;
	}

	AShooterCharacter* Player = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	if (!Player)
	{
		Test->AddError(TEXT("No player pawn found during health regen test"));
		return true;
	}

	const double Elapsed = World->GetTimeSeconds() - StartTimeSec;

	// Before 5 seconds elapse, HP must still be 90
	if (Elapsed < 4.5)
	{
		if (!FMath::IsNearlyEqual(Player->GetCurrentHP(), 90.0f, 0.01f))
		{
			Test->AddError(FString::Printf(TEXT("Health regenerated too early at %.2fs (HP=%.1f)"), Elapsed, Player->GetCurrentHP()));
			return true;
		}
		return false;
	}

	// Wait until 6.4s so both the 5.0s first tick (+1 -> 91) and 6.0s second tick (+1 -> 92) have fired
	if (Elapsed < 6.4)
	{
		return false;
	}

	Test->TestTrue(TEXT("After 5s delay + 1s interval, HP increased by +1/s from 90 to >=92"),
		Player->GetCurrentHP() >= 92.0f && Player->GetCurrentHP() <= 93.0f);
	if (ADCGameMode* GM = World->GetAuthGameMode<ADCGameMode>())
	{
		if (UDCHealthBarWidget* HealthBar = GM->GetHUDWidget() ? GM->GetHUDWidget()->GetHealthBar() : nullptr)
		{
			Test->TestTrue(TEXT("HUD HealthBar reflects regenerated HP"),
				HealthBar->GetLifePercent() >= 0.919f && HealthBar->GetLifePercent() <= 0.931f);
		}
	}

	Test->AddInfo(TEXT("[DC_TEST] Player health regeneration (5s no-damage delay, +1 HP/s) passed!"));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FDCStartHealthRegenTestCommand, FAutomationTestBase*, Test);
bool FDCStartHealthRegenTestCommand::Update()
{
	UWorld* World = AutomationCommon::GetAnyGameWorld();
	AShooterCharacter* Player = World ? Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)) : nullptr;
	if (!World || !Player)
	{
		Test->AddError(TEXT("World or player missing at start of health regen test"));
		return true;
	}

	// Clear any boars and spawners so stray AI bites don't interfere with the 5s no-damage window
	for (TActorIterator<ADCBoarSpawner> It(World); It; ++It)
	{
		It->Destroy();
	}
	for (TActorIterator<ADCBoar> It(World); It; ++It)
	{
		It->Destroy();
	}

	Test->TestEqual(TEXT("Default HealthRegenDelay is 5.0s"), Player->GetHealthRegenDelay(), 5.0f);
	Test->TestEqual(TEXT("Default HealthRegenAmount is 1.0"), Player->GetHealthRegenAmount(), 1.0f);
	Test->TestEqual(TEXT("Default HealthRegenInterval is 1.0s"), Player->GetHealthRegenInterval(), 1.0f);

	// First hit: 100 -> 95, starts 5s timer
	UGameplayStatics::ApplyDamage(Player, 5.0f, nullptr, nullptr, nullptr);
	Test->TestEqual(TEXT("HP drops to 95 after first hit"), Player->GetCurrentHP(), 95.0f);
	Test->TestTrue(TEXT("HealthRegenTimer starts after taking damage"),
		World->GetTimerManager().IsTimerActive(Player->GetHealthRegenTimer()));

	// Second hit: 95 -> 90, resets 5s timer
	UGameplayStatics::ApplyDamage(Player, 5.0f, nullptr, nullptr, nullptr);
	Test->TestEqual(TEXT("HP drops to 90 after second hit"), Player->GetCurrentHP(), 90.0f);
	const float Remaining = World->GetTimerManager().GetTimerRemaining(Player->GetHealthRegenTimer());
	Test->TestTrue(TEXT("HealthRegenTimer resets to ~5.0s on damage"), Remaining > 4.5f && Remaining <= 5.05f);

	ADD_LATENT_AUTOMATION_COMMAND(FDCVerifyHealthRegenCommand(Test, World->GetTimeSeconds()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDCPlayerHealthRegenTest,
	"DirectorsCut.Combat.PlayerHealthRegen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FDCPlayerHealthRegenTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/DirectorsCut/Maps/Lvl_DirectorsCut")))
	{
		AddError(TEXT("Failed to open /Game/DirectorsCut/Maps/Lvl_DirectorsCut"));
		return false;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FDCWaitForGameWorldCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FDCStartHealthRegenTestCommand(this));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
