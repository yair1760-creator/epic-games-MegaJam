// Director's Cut - Beat 4 lockable double doors on the Princess shed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DCShedDoors.generated.h"

class UBoxComponent;
class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDCOnShedDoorLockedAttempt);

/**
 *  Double doors for the Beat 4 Princess Shed.
 *  Starts closed and locked while the King Boar lives; swings open smoothly once unlocked.
 */
UCLASS(DisplayName="DC Shed Doors")
class EPICGAMEJAMGAME_API ADCShedDoors : public AActor
{
	GENERATED_BODY()

public:

	ADCShedDoors();

	/** Locks and closes the doors (called when Beat 4 starts). */
	UFUNCTION(BlueprintCallable, Category="Romance")
	void ResetLocked();

	/** Unlocks the doorway and swings both doors open (called when the King Boar dies). */
	UFUNCTION(BlueprintCallable, Category="Romance")
	void UnlockAndOpen();

	UFUNCTION(BlueprintPure, Category="Romance")
	bool IsLocked() const { return bLocked; }

	UFUNCTION(BlueprintPure, Category="Romance")
	bool IsOpen() const { return bOpening; }

	UPROPERTY(BlueprintAssignable, Category="Romance")
	FDCOnShedDoorLockedAttempt OnLockedAttempt;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> LeftHinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> RightHinge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> LeftDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> RightDoorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> LockBadgeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> LockGlow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> DoorBlocker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> LockPromptTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Romance", meta=(ClampMin=30, ClampMax=140, Units="deg"))
	float OpenAngle = 105.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Romance", meta=(ClampMin=1))
	float OpenSpeed = 5.5f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void HandlePromptOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:

	bool bLocked = true;
	bool bOpening = false;
	float CurrentAngle = 0.0f;
	float LastPromptTime = -100.0f;
};
