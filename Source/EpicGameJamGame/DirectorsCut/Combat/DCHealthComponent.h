// Director's Cut - reusable health component (player-agnostic: boars, dummies, the Director...).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DCHealthComponent.generated.h"

class UDCHealthComponent;
class UDamageType;

DECLARE_LOG_CATEGORY_EXTERN(LogDCCombat, Log, All);

/** HP changed. Delta is negative for damage, positive for healing. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FDCOnHealthChanged, UDCHealthComponent*, HealthComponent, float, CurrentHP, float, Delta, AActor*, DamageCauser);

/** HP reached 0. Fires once per life. Killer is the controller that dealt the final damage (may be null). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDCOnDied, UDCHealthComponent*, HealthComponent, AController*, Killer, AActor*, DamageCauser);

/**
 *  Health that any actor can carry.
 *  - Listens to Unreal's standard damage (ApplyDamage / TakeDamage -> OnTakeAnyDamage), so any weapon,
 *    projectile or trap that uses UGameplayStatics::ApplyDamage works without knowing about this class.
 *  - ApplyDamage / Heal can also be called directly (Blueprint or C++).
 *  - Console (PIE): DC.Damage <amount>, DC.Heal <amount>, DC.Kill  -> act on the actor in front of the player camera.
 */
UCLASS(ClassGroup=(DirectorsCut), meta=(BlueprintSpawnableComponent), DisplayName="DC Health")
class EPICGAMEJAMGAME_API UDCHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UDCHealthComponent();

	/** HP at spawn and after ResetHealth */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin=1))
	float MaxHP = 100.0f;

	/** If true, damage sent to the owning actor (OnTakeAnyDamage) is applied to this component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health")
	bool bListenToActorDamage = true;

	/** If true, damage is ignored (cutscenes, the Director's whims) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Health")
	bool bInvulnerable = false;

	/** Log every hit to the Output Log (LogDCCombat) */
	UPROPERTY(EditAnywhere, Category="Health|Debug")
	bool bLogChanges = true;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FDCOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="Health")
	FDCOnDied OnDied;

	/** Removes HP. Returns the HP actually removed (0 if dead, invulnerable or Amount <= 0). */
	UFUNCTION(BlueprintCallable, Category="Health")
	float ApplyDamage(float Amount, AController* InstigatedBy = nullptr, AActor* DamageCauser = nullptr);

	/** Adds HP up to MaxHP. Does nothing when dead. Returns the HP actually added. */
	UFUNCTION(BlueprintCallable, Category="Health")
	float Heal(float Amount);

	/** Kills instantly (removes all remaining HP) */
	UFUNCTION(BlueprintCallable, Category="Health")
	void Kill(AController* InstigatedBy = nullptr, AActor* DamageCauser = nullptr);

	/** Back to full HP and alive (respawns, dummies) */
	UFUNCTION(BlueprintCallable, Category="Health")
	void ResetHealth();

	UFUNCTION(BlueprintPure, Category="Health")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHP() const { return MaxHP; }

	/** 0..1 */
	UFUNCTION(BlueprintPure, Category="Health")
	float GetHPPercent() const { return MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f; }

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return bDead; }

	/** Finds the health component on an actor (null if none) */
	UFUNCTION(BlueprintPure, Category="Health")
	static UDCHealthComponent* FindHealth(const AActor* Actor);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:

	UPROPERTY(VisibleInstanceOnly, Category="Health")
	float CurrentHP = 0.0f;

	UPROPERTY(VisibleInstanceOnly, Category="Health")
	bool bDead = false;
};
