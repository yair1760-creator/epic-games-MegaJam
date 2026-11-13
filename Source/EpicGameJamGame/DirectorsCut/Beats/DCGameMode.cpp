// Director's Cut - game mode.

#include "Beats/DCGameMode.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Narrator/DCNarratorSubsystem.h"
#include "Quest/DCQuestSubsystem.h"
#include "EpicGameJamGame.h"

ADCGameMode::ADCGameMode()
{
	IntroLines = { FName("L1_Intro_01"), FName("L1_Intro_02"), FName("L1_Intro_03"), FName("L1_Intro_04") };
	HuntQuestTitle = NSLOCTEXT("DirectorsCut", "HuntQuestTitle", "Boars killed");
}

void ADCGameMode::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(IntroTimer, this, &ADCGameMode::StartIntro, FMath::Max(IntroDelay, 0.01f), false);
}

void ADCGameMode::StartIntro()
{
	UDCNarratorSubsystem* Narrator = GetWorld()->GetSubsystem<UDCNarratorSubsystem>();
	if (!Narrator || IntroLines.IsEmpty())
	{
		StartHuntQuest();
		return;
	}

	Narrator->OnLineFinished.AddUniqueDynamic(this, &ADCGameMode::HandleLineFinished);
	Narrator->PlaySequence(IntroLines);
}

void ADCGameMode::HandleLineFinished(FName RowId)
{
	if (!IntroLines.IsEmpty() && RowId == IntroLines.Last())
	{
		StartHuntQuest();
	}
}

void ADCGameMode::StartHuntQuest()
{
	if (bHuntQuestStarted)
	{
		return;
	}
	bHuntQuestStarted = true;

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->StartQuest(HuntQuestId, HuntQuestTitle, HuntQuestTarget, HuntQuestEventTag);
	}

	// TODO(Dev2 hook): enable ADCBoarSpawner here (bActive = true) once it lands.
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Level 1: intro done, hunt quest started"));
}
