// Director's Cut - quest tracking.

#include "Quest/DCQuestSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "EpicGameJamGame.h"

static TAutoConsoleVariable<bool> CVarDCQuestDebugText(
	TEXT("DC.Quest.DebugText"),
	false,
	TEXT("Print quest progress as on-screen debug messages"));

bool UDCQuestSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDCQuestSubsystem::StartQuest(FName Id, FText Title, int32 InTarget, FName EventTag)
{
	SetQuest(Id, Title, InTarget, EventTag, 0);
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Quest started: %s \"%s\" 0/%d on %s"), *ActiveId.ToString(), *ActiveTitle.ToString(), Target, *ActiveEventTag.ToString());
	BroadcastProgress();
}

void UDCQuestSubsystem::ReplaceQuest(FName Id, FText Title, int32 InTarget, FName EventTag, bool bKeepProgress)
{
	const FName OldId = ActiveId;
	SetQuest(Id, Title, InTarget, EventTag, bKeepProgress ? Current : 0);
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Quest replaced: %s -> %s \"%s\" %d/%d on %s"), *OldId.ToString(), *ActiveId.ToString(), *ActiveTitle.ToString(), Current, Target, *ActiveEventTag.ToString());
	OnQuestReplaced.Broadcast(OldId, ActiveId);
	BroadcastProgress();
}

void UDCQuestSubsystem::ReportEvent(FName EventTag)
{
	if (!HasActiveQuest() || bCompleted || EventTag.IsNone() || EventTag != ActiveEventTag)
	{
		UE_LOG(LogEpicGameJamGame, Verbose, TEXT("Quest event %s ignored"), *EventTag.ToString());
		return;
	}

	++Current;
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Quest %s: %d/%d"), *ActiveId.ToString(), Current, Target);
	BroadcastProgress();
}

void UDCQuestSubsystem::SetQuest(FName Id, const FText& Title, int32 InTarget, FName EventTag, int32 StartCount)
{
	ActiveId = Id;
	ActiveTitle = Title;
	ActiveEventTag = EventTag;
	Target = FMath::Max(1, InTarget);
	Current = FMath::Clamp(StartCount, 0, Target);
	bCompleted = false;
}

void UDCQuestSubsystem::BroadcastProgress()
{
	const bool bJustCompleted = !bCompleted && Current >= Target;
	if (bJustCompleted)
	{
		bCompleted = true;
	}

#if !UE_BUILD_SHIPPING
	if (GEngine && CVarDCQuestDebugText.GetValueOnGameThread())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, bJustCompleted ? FColor::Green : FColor::Orange,
			FString::Printf(TEXT("[Quest] %s: %d / %d%s"), *ActiveTitle.ToString(), Current, Target, bJustCompleted ? TEXT("  COMPLETE") : TEXT("")));
	}
#endif

	const FName Id = ActiveId;
	OnQuestUpdated.Broadcast(Id, Current, Target);

	if (bJustCompleted)
	{
		UE_LOG(LogEpicGameJamGame, Log, TEXT("Quest completed: %s"), *Id.ToString());
		OnQuestCompleted.Broadcast(Id);
	}
}

// ---------------------------------------------------------------- debug console commands

/** The Output Log console in the editor passes the editor world, so fall back to the running game / PIE world. */
static UDCQuestSubsystem* FindQuestSubsystem(UWorld* World)
{
	if (UDCQuestSubsystem* Sub = World ? World->GetSubsystem<UDCQuestSubsystem>() : nullptr)
	{
		return Sub;
	}
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* W = Context.World(); W && (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game))
			{
				if (UDCQuestSubsystem* Sub = W->GetSubsystem<UDCQuestSubsystem>())
				{
					return Sub;
				}
			}
		}
	}
	UE_LOG(LogEpicGameJamGame, Warning, TEXT("No quest subsystem: start Play first"));
	return nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs GDCStartQuestCmd(
	TEXT("DC.StartQuest"),
	TEXT("DC.StartQuest <Id> <Target> <EventTag> [Title...] - start a quest, e.g. DC.StartQuest Q_HuntBoars 10 Kill.Boar Boars killed"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("Usage: DC.StartQuest <Id> <Target> <EventTag> [Title...]"));
			return;
		}
		if (UDCQuestSubsystem* Sub = FindQuestSubsystem(World))
		{
			FString Title = Args.Num() > 3 ? FString::Join(TArrayView<const FString>(Args).RightChop(3), TEXT(" ")) : Args[0];
			Sub->StartQuest(FName(*Args[0]), FText::FromString(Title), FCString::Atoi(*Args[1]), FName(*Args[2]));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GDCReportEventCmd(
	TEXT("DC.ReportEvent"),
	TEXT("DC.ReportEvent <EventTag> [Count] - report a quest event, e.g. DC.ReportEvent Kill.Boar 5"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("Usage: DC.ReportEvent <EventTag> [Count]"));
			return;
		}
		if (UDCQuestSubsystem* Sub = FindQuestSubsystem(World))
		{
			const int32 Count = Args.Num() > 1 ? FMath::Max(1, FCString::Atoi(*Args[1])) : 1;
			for (int32 i = 0; i < Count; ++i)
			{
				Sub->ReportEvent(FName(*Args[0]));
			}
		}
	}));
