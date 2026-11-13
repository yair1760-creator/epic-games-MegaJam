// Director's Cut - boar character (body, health, movement settings). The brain is ADCBoarAIController.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DCBoar.generated.h"

class UDCHealthComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UDCFloatingHPBar;
class UDCBillboardSprite;
class ADCBoarMeat;

/** What the boar is doing. Systems 5-6 add Angry / Charge / Bite / Flee / Dead behaviour. */
UENUM(BlueprintType)
enum class EDCBoarState : uint8
{
	Idle,
	Wander,
	Angry,
	Charge,
	Bite,
	Flee,
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCOnBoarStateChanged, class ADCBoar*, Boar, EDCBoarState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDCOnBoarDied, class ADCBoar*, Boar, APawn*, Killer);

/**
 *  Boar body. Placeholder greybox shape until the camera-facing sprite (system 9).
 *  Wander settings live here so each Blueprint / placed boar can be tuned.
 */
UCLASS(DisplayName="DC Boar")
class EPICGAMEJAMGAME_API ADCBoar : public ACharacter
{
	GENERATED_BODY()

public:

	ADCBoar();

	// ---------------- wander

	/** Walking speed while wandering */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Wander", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float WanderSpeed = 180.0f;

	/** Max distance from home for each wander point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Wander", meta=(ClampMin=100, Units="cm"))
	float WanderRadius = 1200.0f;

	/** Shortest step to bother walking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Wander", meta=(ClampMin=0, Units="cm"))
	float MinWanderStep = 300.0f;

	/** Pause between walks (random between min and max) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Wander", meta=(ClampMin=0, Units="s"))
	float IdleTimeMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Wander", meta=(ClampMin=0, Units="s"))
	float IdleTimeMax = 3.0f;

	/** Optional area that keeps the boar in (any actor: its bounding box is used, e.g. a TriggerBox). Spawners set this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Wander")
	TObjectPtr<AActor> WanderArea;

	/** Home point the wander radius is measured from (defaults to where the boar spawned) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Boar|Wander")
	FVector HomeLocation = FVector::ZeroVector;

	// ---------------- anger (system 5)

	/** Seconds the boar stops, faces the attacker and snorts before charging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0, Units="s"))
	float AngryPause = 0.5f;

	/** Running speed while charging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float ChargeSpeed = 650.0f;

	/** Gap between the boar's and the target's capsules at which it bites */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0, Units="cm"))
	float BiteRange = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0))
	float BiteDamage = 40.0f;

	/** Seconds between the boar stopping and the bite landing (the player can step away) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0, Units="s"))
	float BiteWindup = 0.3f;

	/** Seconds after a bite before it can bite again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0, Units="s"))
	float BiteCooldown = 1.0f;

	/** Calms down if the target gets this far away */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0, Units="cm"))
	float LoseInterestDistance = 3000.0f;

	/** Calms down after chasing this long without being hit again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Anger", meta=(ClampMin=0, Units="s"))
	float MaxChaseTime = 12.0f;

	// ---------------- flee + death (system 6)

	/** When a hit leaves HP at or below this fraction of max, the boar runs away instead of fighting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Flee", meta=(ClampMin=0, ClampMax=1))
	float FleeHPFraction = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Flee", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float FleeSpeed = 520.0f;

	/** Stops fleeing once this far from the attacker */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Flee", meta=(ClampMin=0, Units="cm"))
	float SafeDistance = 1600.0f;

	/** Gives up fleeing after this long (then wanders, still hurt) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Flee", meta=(ClampMin=0, Units="s"))
	float MaxFleeTime = 6.0f;

	/** Pickup dropped on death while DCBoarSubsystem meat drops are on (BP_DC_BoarMeat) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Meat")
	TSubclassOf<ADCBoarMeat> MeatClass;

	/** Chance (0..1) to drop meat when drops are on */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Meat", meta=(ClampMin=0, ClampMax=1))
	float MeatDropChance = 1.0f;

	/** Seconds the body stays on the ground before it is removed (0 = stays forever) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Death", meta=(ClampMin=0, Units="s"))
	float CorpseTime = 4.0f;

	// ---------------- debug

	/** Show the 2.5D sprite and hide the greybox body (off = greybox boxes, e.g. if the sprite art is missing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Look")
	bool bUseSprite = true;

	/** Floating state label above the boar (test map) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boar|Debug")
	bool bShowDebugState = true;

	// ---------------- state

	UFUNCTION(BlueprintPure, Category="Boar")
	EDCBoarState GetBoarState() const { return BoarState; }

	/** Called by the AI controller */
	void SetBoarState(EDCBoarState NewState);

	UPROPERTY(BlueprintAssignable, Category="Boar")
	FDCOnBoarStateChanged OnStateChanged;

	UFUNCTION(BlueprintPure, Category="Boar")
	UDCHealthComponent* GetHealth() const { return Health; }

	/** Fires once when this boar dies. Killer = pawn that dealt the final blow (may be null).
	 *  To count every boar in the level, bind DCBoarSubsystem.OnBoarKilled instead. */
	UPROPERTY(BlueprintAssignable, Category="Boar")
	FDCOnBoarDied OnBoarDied;

	UFUNCTION(BlueprintPure, Category="Boar")
	bool IsDead() const { return BoarState == EDCBoarState::Dead; }

	/** True if Location is inside the wander area (always true when there is none) */
	bool IsInsideWanderArea(const FVector& Location) const;

	/** Random point inside the wander area and radius (not projected to the nav mesh yet) */
	FVector PickRandomWanderPoint() const;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCHealthComponent> Health;

	/** Greybox body parts (replaced by the sprite later) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UTextRenderComponent> StateText;

	/** Camera-facing 2.5D sprite (system 9). Set its Front / Side textures on BP_DC_Boar. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCBillboardSprite> Sprite;

	/** HP bar over the boar (system 8): hidden at full HP */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UDCFloatingHPBar> HPBar;

	UFUNCTION()
	void HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser);

	UFUNCTION()
	void HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser);

	/** Picks the sprite animation from the state and movement speed (every frame) */
	void UpdateSpriteAnimation();

private:

	UPROPERTY(VisibleInstanceOnly, Category="Boar")
	EDCBoarState BoarState = EDCBoarState::Idle;

	void UpdateStateText();

	/** Death already handled (quest reported, events fired) */
	bool bDeathHandled = false;

	/** Death tip-over animation progress (< 0 = not dying) */
	float DeathAnimTime = -1.0f;
	FRotator DeathStartRotation = FRotator::ZeroRotator;
	FTimerHandle CorpseTimer;
};
