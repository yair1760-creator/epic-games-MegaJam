// Director's Cut - boar events.

#include "DirectorsCut/AI/DCBoarSubsystem.h"
#include "DirectorsCut/AI/DCBoar.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

int32 UDCBoarSubsystem::GetAliveCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ADCBoar>& B : Alive)
	{
		N += B.IsValid() ? 1 : 0;
	}
	return N;
}

TArray<ADCBoar*> UDCBoarSubsystem::GetAliveBoars() const
{
	TArray<ADCBoar*> Out;
	for (const TWeakObjectPtr<ADCBoar>& B : Alive)
	{
		if (B.IsValid())
		{
			Out.Add(B.Get());
		}
	}
	return Out;
}

void UDCBoarSubsystem::RegisterBoar(ADCBoar* Boar)
{
	Alive.RemoveAll([](const TWeakObjectPtr<ADCBoar>& B) { return !B.IsValid(); });
	Alive.AddUnique(Boar);
	OnBoarSpawned.Broadcast(Boar);
}

void UDCBoarSubsystem::SetMeatDropsEnabled(bool bEnabled)
{
	if (bMeatDropsEnabled != bEnabled)
	{
		bMeatDropsEnabled = bEnabled;
		UE_LOG(LogDCCombat, Log, TEXT("Boar meat drops %s"), bEnabled ? TEXT("ON") : TEXT("OFF"));
	}
}

void UDCBoarSubsystem::NotifyMeatCollected(AActor* Meat, APawn* Collector)
{
	UE_LOG(LogDCCombat, Log, TEXT("Meat collected by %s"), *GetNameSafe(Collector));
	OnMeatCollected.Broadcast(Meat, Collector);
}

static FAutoConsoleCommandWithWorldAndArgs GDCBoarMeatDropsCmd(
	TEXT("DC.BoarMeatDrops"),
	TEXT("DC.BoarMeatDrops 1|0 - boars drop meat pickups on death (the meat phase of the hunt)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (UDCBoarSubsystem* Boars = World ? World->GetSubsystem<UDCBoarSubsystem>() : nullptr)
		{
			Boars->SetMeatDropsEnabled(Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0);
		}
	}));

void UDCBoarSubsystem::NotifyBoarKilled(ADCBoar* Boar, APawn* Killer)
{
	Alive.Remove(Boar);
	++KilledCount;
	UE_LOG(LogDCCombat, Log, TEXT("Boar killed: %s by %s (killed %d, alive %d)"),
		*GetNameSafe(Boar), *GetNameSafe(Killer), KilledCount, GetAliveCount());
	OnBoarKilled.Broadcast(Boar, Killer);
}
