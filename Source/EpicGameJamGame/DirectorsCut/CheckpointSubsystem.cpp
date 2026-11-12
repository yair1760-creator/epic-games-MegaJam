// Director's Cut - checkpoint tracking.

#include "DirectorsCut/CheckpointSubsystem.h"
#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EpicGameJamGame.h"

const FName UCheckpointSubsystem::CheckpointTag(TEXT("Checkpoint"));
const FName UCheckpointSubsystem::RespawnTag(TEXT("Respawn"));

bool UCheckpointSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

int32 UCheckpointSubsystem::ParseIndex(const AActor* Actor)
{
	for (const FName& Tag : Actor->Tags)
	{
		const FString S = Tag.ToString();
		if (S.StartsWith(TEXT("CP_")))
		{
			const FString Num = S.RightChop(3);
			if (Num.IsNumeric())
			{
				return FCString::Atoi(*Num);
			}
		}
	}
	return INDEX_NONE;
}

void UCheckpointSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	Checkpoints.Reset();
	ActiveIndex = INDEX_NONE;

	for (TActorIterator<AActor> It(&InWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor->ActorHasTag(CheckpointTag))
		{
			continue;
		}

		const int32 Index = ParseIndex(Actor);
		if (Index == INDEX_NONE)
		{
			UE_LOG(LogEpicGameJamGame, Warning, TEXT("Checkpoint actor %s has no CP_<NN> tag"), *Actor->GetName());
			continue;
		}

		FDirectorCheckpoint& CP = Checkpoints.FindOrAdd(Index);
		CP.Index = Index;

		if (Actor->ActorHasTag(RespawnTag))
		{
			CP.RespawnTransform = FTransform(FRotator(0.0, Actor->GetActorRotation().Yaw, 0.0), Actor->GetActorLocation());
		}
		else
		{
			CP.Trigger = Actor;
			Actor->OnActorBeginOverlap.AddUniqueDynamic(this, &UCheckpointSubsystem::HandleTriggerOverlap);

			// fall back to the trigger position until/unless a respawn point is found
			if (CP.RespawnTransform.Equals(FTransform::Identity))
			{
				CP.RespawnTransform = FTransform(FRotator(0.0, Actor->GetActorRotation().Yaw, 0.0), Actor->GetActorLocation());
			}
		}
	}

	if (Checkpoints.Num() > 0)
	{
		TArray<int32> Keys = GetCheckpointIndices();
		ActiveIndex = Keys[0];
	}

	UE_LOG(LogEpicGameJamGame, Log, TEXT("CheckpointSubsystem: %d checkpoints, start at %d"), Checkpoints.Num(), ActiveIndex);
}

TArray<int32> UCheckpointSubsystem::GetCheckpointIndices() const
{
	TArray<int32> Keys;
	Checkpoints.GetKeys(Keys);
	Keys.Sort();
	return Keys;
}

FTransform UCheckpointSubsystem::GetRespawnTransform() const
{
	if (const FDirectorCheckpoint* CP = Checkpoints.Find(ActiveIndex))
	{
		return CP->RespawnTransform;
	}
	return FTransform::Identity;
}

bool UCheckpointSubsystem::ActivateCheckpoint(int32 Index, bool bForce)
{
	const FDirectorCheckpoint* CP = Checkpoints.Find(Index);
	if (!CP)
	{
		return false;
	}
	if (!bForce && Index <= ActiveIndex)
	{
		return false;
	}

	ActiveIndex = Index;
	UE_LOG(LogEpicGameJamGame, Log, TEXT("Checkpoint CP_%02d activated"), Index);

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Cyan, FString::Printf(TEXT("Checkpoint CP_%02d"), Index));
	}
#endif

	OnCheckpointActivated.Broadcast(Index, CP->Trigger);
	return true;
}

void UCheckpointSubsystem::HandleTriggerOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}
	ActivateCheckpoint(ParseIndex(OverlappedActor));
}

bool UCheckpointSubsystem::TeleportPlayerToCheckpoint(APlayerController* PlayerController, int32 Index)
{
	if (!PlayerController || !Checkpoints.Contains(Index))
	{
		return false;
	}

	ActivateCheckpoint(Index, true);
	const FTransform T = GetRespawnTransform();

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		Pawn->TeleportTo(T.GetLocation(), T.Rotator(), false, true);
	}
	PlayerController->SetControlRotation(T.Rotator());
	return true;
}

static FAutoConsoleCommandWithWorldAndArgs GDCGotoCheckpointCmd(
	TEXT("DC.GotoCheckpoint"),
	TEXT("DC.GotoCheckpoint <N> - teleport the local player to checkpoint N and make it active"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 1)
		{
			return;
		}
		if (UCheckpointSubsystem* Sub = World->GetSubsystem<UCheckpointSubsystem>())
		{
			Sub->TeleportPlayerToCheckpoint(World->GetFirstPlayerController(), FCString::Atoi(*Args[0]));
		}
	}));
