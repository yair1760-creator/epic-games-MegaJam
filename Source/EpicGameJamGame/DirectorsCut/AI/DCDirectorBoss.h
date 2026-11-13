// Director's Cut - Beat 5 final boss: The Director enters the scene.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DCDirectorBoss.generated.h"

class UDCHealthComponent;
class UDCFloatingHPBar;
class UStaticMeshComponent;
class UPointLightComponent;
class UTextRenderComponent;

class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnDirectorBossEvent);

UCLASS(DisplayName="DC Director Boss")
class EPICGAMEJAMGAME_API ADCDirectorBoss : public ACharacter
{
	GENERATED_BODY()

public:

	ADCDirectorBoss();

	UFUNCTION(BlueprintCallable, Category="Boss")
	void StartFight();

	UFUNCTION(BlueprintPure, Category="Boss")
	bool IsFightActive() const { return bFightActive && !bDead; }

	UFUNCTION(BlueprintPure, Category="Boss")
	UDCHealthComponent* GetHealth() const { return Health; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
	FVector ArenaCenter = FVector(1000.0f, 0.0f, 40.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin=0.2))
	float FireInterval = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin=1.0))
	float TauntInterval = 9.0f;

	// ---------------- audio / VFX (empty = silent / nothing; placeholders in /Game/DirectorsCut/Audio/SFX)

	/** Each time he fires (once per volley) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Audio")
	TObjectPtr<USoundBase> AttackSound;

	/** When he takes damage (at most every 0.15 s; the rifle hits 10 times a second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Audio")
	TObjectPtr<USoundBase> HitSound;

	/** When he is defeated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Audio")
	TObjectPtr<USoundBase> DefeatSound;

	/** Effect when he is defeated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|FX")
	TObjectPtr<UNiagaraSystem> DefeatFX;

	UPROPERTY(BlueprintAssignable, Category="Boss")
	FDCOnDirectorBossEvent OnBossHalfHP;

	UPROPERTY(BlueprintAssignable, Category="Boss")
	FDCOnDirectorBossEvent OnBossTaunt;

	UPROPERTY(BlueprintAssignable, Category="Boss")
	FDCOnDirectorBossEvent OnBossDied;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCFloatingHPBar> HPBar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> CoatMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BeretMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> MegaphoneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> SpotlightAura;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> NameText;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser);

private:

	void FireAtPlayer();
	void SpawnProjectileTowards(const FVector& TargetPoint, float YawOffsetDeg = 0.0f);
	void TriggerTaunt();

	bool bFightActive = false;
	bool bDead = false;
	bool bHalfHPTriggered = false;
	float OrbitAngle = 0.0f;
	float OrbitDir = 1.0f;

	FTimerHandle FireTimer;
	FTimerHandle TauntTimer;
};
