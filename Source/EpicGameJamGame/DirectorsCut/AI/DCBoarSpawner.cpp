// Director's Cut - boar spawner.

#include "DirectorsCut/AI/DCBoarSpawner.h"
#include "DirectorsCut/AI/DCBoar.h"
#include "DirectorsCut/AI/DCBoarAIController.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ADCBoarSpawner::ADCBoarSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	Icon = CreateDefaultSubobject<UBillboardComponent>(TEXT("Icon"));
	SetRootComponent(Icon);
	Icon->bIsScreenSizeScaled = true;

	static ConstructorHelpers::FClassFinder<ADCBoar> BoarBP(TEXT("/Game/DirectorsCut/AI/BP_DC_Boar"));
	BoarClass = BoarBP.Succeeded() ? BoarBP.Class : TSubclassOf<ADCBoar>(ADCBoar::StaticClass());
}

void ADCBoarSpawner::BeginPlay()
{
	Super::BeginPlay();

	GatherSpawnPoints();
	if (bAutoStart)
	{
		StartSpawning();
	}
}

void ADCBoarSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FillTimer);
	Super::EndPlay(EndPlayReason);
}

void ADCBoarSpawner::GatherSpawnPoints()
{
	FoundPoints.Reset();
	for (AActor* P : SpawnPoints)
	{
		if (P)
		{
			FoundPoints.Add(P);
		}
	}
	if (FoundPoints.Num() == 0 && !SpawnPointTag.IsNone())
	{
		TArray<AActor*> Tagged;
		UGameplayStatics::GetAllActorsWithTag(this, SpawnPointTag, Tagged);
		for (AActor* P : Tagged)
		{
			FoundPoints.Add(P);
		}
	}
	UE_LOG(LogDCCombat, Log, TEXT("%s: %d spawn points"), *GetName(), FoundPoints.Num());
}

int32 ADCBoarSpawner::GetAliveCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ADCBoar>& B : Living)
	{
		N += (B.IsValid() && !B->IsDead()) ? 1 : 0;
	}
	return N;
}

bool ADCBoarSpawner::CanSpawnMore() const
{
	return bSpawning && BoarClass && FoundPoints.Num() > 0 && GetAliveCount() < MaxAlive
		&& (TotalToSpawn <= 0 || SpawnedTotal < TotalToSpawn);
}

void ADCBoarSpawner::StartSpawning()
{
	bSpawning = true;
	if (FoundPoints.Num() == 0)
	{
		GatherSpawnPoints();
	}
	GetWorldTimerManager().SetTimer(FillTimer, this, &ADCBoarSpawner::FillUp, 0.05f, false);
}

void ADCBoarSpawner::StopSpawning(bool bDespawnExisting)
{
	bSpawning = false;
	GetWorldTimerManager().ClearTimer(FillTimer);
	if (bDespawnExisting)
	{
		DespawnAll();
	}
}

void ADCBoarSpawner::DespawnAll()
{
	for (const TWeakObjectPtr<ADCBoar>& B : Living)
	{
		if (B.IsValid())
		{
			B->Destroy();
		}
	}
	Living.Reset();
}

void ADCBoarSpawner::SetHyperspawnMode(bool bEnabled)
{
	if (!bSavedNormalDefaults)
	{
		bSavedNormalDefaults = true;
		NormalMaxAlive = MaxAlive;
		NormalRespawnDelay = RespawnDelay;
		NormalStaggerDelay = StaggerDelay;
		NormalMinPlayerDistance = MinPlayerDistance;
	}

	bHyperspawnMode = bEnabled;
	if (bEnabled)
	{
		MaxAlive = 12;
		RespawnDelay = 0.25f;
		StaggerDelay = 0.25f;
		MinPlayerDistance = 850.0f;
		bAutoAggroNewBoars = true;
		OverrideBoarMaxHP = 25.0f;
		OverrideCorpseTime = 1.0f;

		// Make existing living boars charge immediately too
		for (const TWeakObjectPtr<ADCBoar>& B : Living)
		{
			if (ADCBoar* Boar = B.Get(); Boar && !Boar->IsDead())
			{
				Boar->bAutoAggroPlayer = true;
				Boar->CorpseTime = 1.0f;
				if (UDCHealthComponent* H = Boar->GetHealth())
				{
					H->MaxHP = 25.0f;
					H->ResetHealth();
				}
				if (ADCBoarAIController* AI = Cast<ADCBoarAIController>(Boar->GetController()))
				{
					if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
					{
						AI->BecomeAngry(Player);
					}
				}
			}
		}

		StartSpawning();
	}
	else
	{
		MaxAlive = NormalMaxAlive;
		RespawnDelay = NormalRespawnDelay;
		StaggerDelay = NormalStaggerDelay;
		MinPlayerDistance = NormalMinPlayerDistance;
		bAutoAggroNewBoars = false;
		OverrideBoarMaxHP = 0.0f;
		OverrideCorpseTime = 0.0f;
	}
}

void ADCBoarSpawner::FillUp()
{
	if (!CanSpawnMore())
	{
		return;
	}

	SpawnOne();   // may fail if every point is blocked; try again shortly either way

	if (CanSpawnMore())
	{
		GetWorldTimerManager().SetTimer(FillTimer, this, &ADCBoarSpawner::FillUp, FMath::Max(StaggerDelay, 0.05f), false);
	}
}

AActor* ADCBoarSpawner::PickSpawnPoint() const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);

	TArray<AActor*> Good;
	TArray<AActor*> NotTooClose;
	for (AActor* P : FoundPoints)
	{
		if (!P)
		{
			continue;
		}
		const FVector L = P->GetActorLocation();
		if (Player && FVector::Dist2D(L, Player->GetActorLocation()) < MinPlayerDistance)
		{
			continue;
		}
		NotTooClose.Add(P);
		bool bOccupied = false;
		for (const TWeakObjectPtr<ADCBoar>& B : Living)
		{
			if (B.IsValid() && FVector::Dist2D(B->GetActorLocation(), L) < 150.0f)
			{
				bOccupied = true;
				break;
			}
		}
		if (!bOccupied)
		{
			Good.Add(P);
		}
	}
	if (Good.Num() > 0)
	{
		return Good[FMath::RandRange(0, Good.Num() - 1)];
	}
	if (bHyperspawnMode && NotTooClose.Num() > 0)
	{
		return NotTooClose[FMath::RandRange(0, NotTooClose.Num() - 1)];
	}
	return nullptr;
}

ADCBoar* ADCBoarSpawner::SpawnOne()
{
	AActor* Point = PickSpawnPoint();
	if (!Point)
	{
		return nullptr;
	}

	// spawn standing on the point, facing the way the point faces (with jitter in hyperspawn)
	const float HalfHeight = BoarClass->GetDefaultObject<ADCBoar>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Jitter = bHyperspawnMode
		? FVector(FMath::FRandRange(-180.0f, 180.0f), FMath::FRandRange(-180.0f, 180.0f), 0.0f)
		: FVector::ZeroVector;
	FTransform T(FRotator(0.0f, Point->GetActorRotation().Yaw, 0.0f), Point->GetActorLocation() + Jitter + FVector(0.0f, 0.0f, HalfHeight + 5.0f));

	ADCBoar* Boar = GetWorld()->SpawnActorDeferred<ADCBoar>(BoarClass, T, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Boar)
	{
		return nullptr;
	}
	if (WanderArea)
	{
		Boar->WanderArea = WanderArea;
	}
	if (bAutoAggroNewBoars)
	{
		Boar->bAutoAggroPlayer = true;
		Boar->bShowDebugState = false;
	}
	if (OverrideCorpseTime > 0.0f)
	{
		Boar->CorpseTime = OverrideCorpseTime;
	}
	Boar->FinishSpawning(T);
	if (!IsValid(Boar))
	{
		return nullptr;
	}
	if (OverrideBoarMaxHP > 0.0f)
	{
		if (UDCHealthComponent* H = Boar->GetHealth())
		{
			H->MaxHP = OverrideBoarMaxHP;
			H->ResetHealth();
		}
	}

	Boar->OnBoarDied.AddUniqueDynamic(this, &ADCBoarSpawner::HandleBoarDied);
	Living.RemoveAll([](const TWeakObjectPtr<ADCBoar>& B) { return !B.IsValid(); });
	Living.Add(Boar);
	++SpawnedTotal;
	return Boar;
}

void ADCBoarSpawner::HandleBoarDied(ADCBoar* Boar, APawn* Killer)
{
	if (bSpawning && CanSpawnMore() && !GetWorldTimerManager().IsTimerActive(FillTimer))
	{
		GetWorldTimerManager().SetTimer(FillTimer, this, &ADCBoarSpawner::FillUp, FMath::Max(RespawnDelay, 0.05f), false);
	}
}
