// Director's Cut - boar events.

#include "DirectorsCut/AI/DCBoarSubsystem.h"
#include "DirectorsCut/AI/DCBoar.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"

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

void UDCBoarSubsystem::NotifyBoarKilled(ADCBoar* Boar, APawn* Killer)
{
	Alive.Remove(Boar);
	++KilledCount;
	UE_LOG(LogDCCombat, Log, TEXT("Boar killed: %s by %s (killed %d, alive %d)"),
		*GetNameSafe(Boar), *GetNameSafe(Killer), KilledCount, GetAliveCount());
	OnBoarKilled.Broadcast(Boar, Killer);
}
