// Director's Cut - the Director's voice.

#include "Narrator/DCNarratorSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "EpicGameJamGame.h"

static TAutoConsoleVariable<bool> CVarDCNarratorDebugText(
	TEXT("DC.Narrator.DebugText"),
	false,
	TEXT("Print Director lines as on-screen debug messages"));

bool UDCNarratorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDCNarratorSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LineTimer);
	}
	StopVoice();
	Queue.Reset();
	CurrentLineId = NAME_None;
	Super::Deinitialize();
}

void UDCNarratorSubsystem::PlayLine(FName RowId)
{
	if (RowId.IsNone())
	{
		return;
	}
	Queue.Add(RowId);
	if (!IsPlaying())
	{
		StartNextLine();
	}
}

void UDCNarratorSubsystem::PlaySequence(const TArray<FName>& RowIds)
{
	for (const FName& RowId : RowIds)
	{
		if (!RowId.IsNone())
		{
			Queue.Add(RowId);
		}
	}
	if (!IsPlaying())
	{
		StartNextLine();
	}
}

void UDCNarratorSubsystem::PlayRandomLine(const TArray<FName>& RowIds)
{
	TArray<FName> Valid;
	for (const FName& Id : RowIds)
	{
		if (!Id.IsNone())
		{
			Valid.Add(Id);
		}
	}
	if (Valid.Num() > 0)
	{
		PlayLine(Valid[FMath::RandRange(0, Valid.Num() - 1)]);
	}
}

void UDCNarratorSubsystem::PlayAlternatingLine(FName PrimaryRowId, FName AltRowId)
{
	if (PrimaryRowId.IsNone())
	{
		PlayLine(AltRowId);
		return;
	}
	if (AltRowId.IsNone())
	{
		PlayLine(PrimaryRowId);
		return;
	}

	bool& bUseAlt = AlternateToggle.FindOrAdd(PrimaryRowId, false);
	const FName Chosen = bUseAlt ? AltRowId : PrimaryRowId;
	bUseAlt = !bUseAlt;
	PlayLine(Chosen);
}

void UDCNarratorSubsystem::FindRowNamesForDebug()
{
	if (!LoadedTable)
	{
		LoadedTable = LinesTable.LoadSynchronous();
	}
	if (!LoadedTable)
	{
		UE_LOG(LogEpicGameJamGame, Error, TEXT("Narrator: lines table '%s' not found"), *LinesTable.ToString());
		return;
	}
	for (const FName& Name : LoadedTable->GetRowNames())
	{
		UE_LOG(LogEpicGameJamGame, Log, TEXT("Narrator row: %s"), *Name.ToString());
	}
}

void UDCNarratorSubsystem::Interrupt()
{
	Queue.Reset();
	if (!IsPlaying())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(LineTimer);
	StopVoice();
	const FName CutLine = CurrentLineId;
	CurrentLineId = NAME_None;
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Narrator: interrupted %s"), *CutLine.ToString());
	OnLineFinished.Broadcast(CutLine);
}

const FDCNarratorLine* UDCNarratorSubsystem::FindLine(FName RowId)
{
	if (!LoadedTable)
	{
		LoadedTable = LinesTable.LoadSynchronous();
		if (!LoadedTable)
		{
			UE_LOG(LogEpicGameJamGame, Error, TEXT("Narrator: lines table '%s' not found (DefaultGame.ini, DCNarratorSubsystem.LinesTable)"), *LinesTable.ToString());
			return nullptr;
		}
	}
	return LoadedTable->FindRow<FDCNarratorLine>(RowId, TEXT("Narrator"), false);
}

void UDCNarratorSubsystem::StartNextLine()
{
	while (Queue.Num() > 0)
	{
		const FName RowId = Queue[0];
		Queue.RemoveAt(0);

		const FDCNarratorLine* Line = FindLine(RowId);
		if (!Line)
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("Narrator: no line '%s' in the lines table, skipped"), *RowId.ToString());
			continue;
		}

		float Duration = Line->Duration;
		if (Line->Voice)
		{
			const float VoiceLength = Line->Voice->GetDuration();
			if (VoiceLength < INDEFINITELY_LOOPING_DURATION)
			{
				Duration = FMath::Max(Duration, VoiceLength);
			}
			VoiceComponent = UGameplayStatics::SpawnSound2D(this, Line->Voice, VoiceVolume);
		}
		Duration = FMath::Max(Duration, 0.1f);

		CurrentLineId = RowId;
		GetWorld()->GetTimerManager().SetTimer(LineTimer, this, &UDCNarratorSubsystem::FinishCurrentLine, Duration, false);

		UE_LOG(LogEpicGameJamGame, Log, TEXT("Narrator: %s (%.1fs) %s: %s"), *RowId.ToString(), Duration, *Line->Speaker.ToString(), *Line->Text.ToString());
#if !UE_BUILD_SHIPPING
		if (GEngine && CVarDCNarratorDebugText.GetValueOnGameThread())
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, FColor::Yellow, FString::Printf(TEXT("%s: %s"), *Line->Speaker.ToString(), *Line->Text.ToString()));
		}
#endif

		OnLineStarted.Broadcast(Line->Speaker, Line->Text, Duration);
		return;
	}
}

void UDCNarratorSubsystem::FinishCurrentLine()
{
	const FName Finished = CurrentLineId;
	CurrentLineId = NAME_None;
	VoiceComponent = nullptr;
	OnLineFinished.Broadcast(Finished);

	// a listener may already have started the next line
	if (!IsPlaying())
	{
		StartNextLine();
	}
}

void UDCNarratorSubsystem::StopVoice()
{
	if (VoiceComponent)
	{
		VoiceComponent->Stop();
		VoiceComponent = nullptr;
	}
}

// ---------------------------------------------------------------- debug console commands

/** The Output Log console in the editor passes the editor world, so fall back to the running game / PIE world. */
static UDCNarratorSubsystem* FindNarratorSubsystem(UWorld* World)
{
	if (UDCNarratorSubsystem* Sub = World ? World->GetSubsystem<UDCNarratorSubsystem>() : nullptr)
	{
		return Sub;
	}
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* W = Context.World(); W && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				if (UDCNarratorSubsystem* Sub = W->GetSubsystem<UDCNarratorSubsystem>())
				{
					return Sub;
				}
			}
		}
	}
	UE_LOG(LogEpicGameJamGame, Warning, TEXT("No narrator subsystem: start Play first"));
	return nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs GDCPlayLineCmd(
	TEXT("DC.PlayLine"),
	TEXT("DC.PlayLine <RowId> [RowId...] - queue Director lines from DT_NarratorLines, e.g. DC.PlayLine L1_Intro_01 L1_Intro_02"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("Usage: DC.PlayLine <RowId> [RowId...]"));
			return;
		}
		if (UDCNarratorSubsystem* Sub = FindNarratorSubsystem(World))
		{
			TArray<FName> Rows;
			for (const FString& Arg : Args)
			{
				Rows.Add(FName(*Arg));
			}
			Sub->PlaySequence(Rows);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDCInterruptLineCmd(
	TEXT("DC.InterruptLine"),
	TEXT("DC.InterruptLine - cut off the current Director line and clear the queue"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UDCNarratorSubsystem* Sub = FindNarratorSubsystem(World))
		{
			Sub->Interrupt();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDCListLinesCmd(
	TEXT("DC.ListLines"),
	TEXT("DC.ListLines - print every RowId in DT_NarratorLines"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UDCNarratorSubsystem* Sub = FindNarratorSubsystem(World))
		{
			// Trigger load if needed
			Sub->FindRowNamesForDebug();
		}
	}));
