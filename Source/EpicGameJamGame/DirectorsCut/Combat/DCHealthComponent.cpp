// Director's Cut - reusable health component.

#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogDCCombat);

UDCHealthComponent::UDCHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDCHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	bDead = false;

	if (bListenToActorDamage)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &UDCHealthComponent::HandleTakeAnyDamage);
		}
	}
}

void UDCHealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	ApplyDamage(Damage, InstigatedBy, DamageCauser);
}

float UDCHealthComponent::ApplyDamage(float Amount, AController* InstigatedBy, AActor* DamageCauser)
{
	if (bDead || bInvulnerable || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float Applied = FMath::Min(Amount, CurrentHP);
	CurrentHP -= Applied;

	if (bLogChanges)
	{
		UE_LOG(LogDCCombat, Log, TEXT("%s took %.1f damage from %s -> %.1f / %.1f HP"),
			*GetNameSafe(GetOwner()), Applied, *GetNameSafe(DamageCauser), CurrentHP, MaxHP);
	}

	OnHealthChanged.Broadcast(this, CurrentHP, -Applied, DamageCauser);

	if (CurrentHP <= 0.0f)
	{
		CurrentHP = 0.0f;
		bDead = true;

		if (bLogChanges)
		{
			UE_LOG(LogDCCombat, Log, TEXT("%s died (killer: %s)"), *GetNameSafe(GetOwner()), *GetNameSafe(InstigatedBy));
		}

		OnDied.Broadcast(this, InstigatedBy, DamageCauser);
	}

	return Applied;
}

float UDCHealthComponent::Heal(float Amount)
{
	if (bDead || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float Added = FMath::Min(Amount, MaxHP - CurrentHP);
	if (Added <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHP += Added;

	if (bLogChanges)
	{
		UE_LOG(LogDCCombat, Log, TEXT("%s healed %.1f -> %.1f / %.1f HP"), *GetNameSafe(GetOwner()), Added, CurrentHP, MaxHP);
	}

	OnHealthChanged.Broadcast(this, CurrentHP, Added, nullptr);
	return Added;
}

void UDCHealthComponent::Kill(AController* InstigatedBy, AActor* DamageCauser)
{
	const bool bWasInvulnerable = bInvulnerable;
	bInvulnerable = false;
	ApplyDamage(CurrentHP, InstigatedBy, DamageCauser);
	bInvulnerable = bWasInvulnerable;
}

void UDCHealthComponent::ResetHealth()
{
	const float Delta = MaxHP - CurrentHP;
	CurrentHP = MaxHP;
	bDead = false;
	OnHealthChanged.Broadcast(this, CurrentHP, Delta, nullptr);
}

UDCHealthComponent* UDCHealthComponent::FindHealth(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UDCHealthComponent>() : nullptr;
}

// ---------------------------------------------------------------- debug console commands (PIE)

namespace DCHealthDebug
{
	/** Actor the first local player is looking at (up to 50 m) */
	static AActor* GetLookedAtActor(UWorld* World, APlayerController*& OutPC)
	{
		OutPC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
		if (!OutPC)
		{
			return nullptr;
		}

		FVector ViewLoc;
		FRotator ViewRot;
		OutPC->GetPlayerViewPoint(ViewLoc, ViewRot);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(DCHealthDebug), false, OutPC->GetPawn());
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, ViewLoc, ViewLoc + ViewRot.Vector() * 5000.0f, ECC_Visibility, Params))
		{
			return Hit.GetActor();
		}
		return nullptr;
	}

	static UDCHealthComponent* GetTarget(UWorld* World, APlayerController*& OutPC)
	{
		AActor* Target = GetLookedAtActor(World, OutPC);
		UDCHealthComponent* Health = UDCHealthComponent::FindHealth(Target);
		if (!Health)
		{
			UE_LOG(LogDCCombat, Warning, TEXT("No actor with a DC Health component in front of the camera (hit: %s)"), *GetNameSafe(Target));
		}
		return Health;
	}

	static float ParseAmount(const TArray<FString>& Args, float Default)
	{
		return Args.Num() > 0 ? FCString::Atof(*Args[0]) : Default;
	}

	static FAutoConsoleCommandWithWorldAndArgs DamageCmd(
		TEXT("DC.Damage"),
		TEXT("DC.Damage <amount=25>: deals standard Unreal damage to the actor in front of the camera"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APlayerController* PC = nullptr;
			if (UDCHealthComponent* Health = GetTarget(World, PC))
			{
				// go through the standard damage path on purpose, so this also tests the OnTakeAnyDamage listener
				UGameplayStatics::ApplyDamage(Health->GetOwner(), ParseAmount(Args, 25.0f), PC, PC ? PC->GetPawn() : nullptr, nullptr);
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs HealCmd(
		TEXT("DC.Heal"),
		TEXT("DC.Heal <amount=25>: heals the actor in front of the camera"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APlayerController* PC = nullptr;
			if (UDCHealthComponent* Health = GetTarget(World, PC))
			{
				Health->Heal(ParseAmount(Args, 25.0f));
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs HurtMeCmd(
		TEXT("DC.HurtMe"),
		TEXT("DC.HurtMe <amount=50>: deals standard Unreal damage to the local player"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr)
			{
				UGameplayStatics::ApplyDamage(Pawn, ParseAmount(Args, 50.0f), nullptr, nullptr, nullptr);
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs KillCmd(
		TEXT("DC.Kill"),
		TEXT("DC.Kill: kills the actor in front of the camera"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APlayerController* PC = nullptr;
			if (UDCHealthComponent* Health = GetTarget(World, PC))
			{
				Health->Kill(PC, PC ? PC->GetPawn() : nullptr);
			}
		}));
}
