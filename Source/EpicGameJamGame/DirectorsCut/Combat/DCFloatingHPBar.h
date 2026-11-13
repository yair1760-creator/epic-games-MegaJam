// Director's Cut - small HP bar that floats over an actor with a DC Health component (boars now, the Director later).

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DCFloatingHPBar.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UDCHealthComponent;

/**
 *  Attach to an actor that has a DC Health component. Built from three flat unlit strips
 *  (frame, trailing "recent damage" strip, fill), so no UMG widget asset is needed.
 *  - Hidden at full HP, appears on the first hit, hides again when healed to full or on death.
 *  - Always turns to face the player camera.
 *  - The fill drops instantly; a light strip behind it drains after a short pause, so you can read how big the hit was.
 */
UCLASS(ClassGroup=(DirectorsCut), meta=(BlueprintSpawnableComponent), DisplayName="DC Floating HP Bar")
class EPICGAMEJAMGAME_API UDCFloatingHPBar : public USceneComponent
{
	GENERATED_BODY()

public:

	UDCFloatingHPBar();

	/** Bar size in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar", meta=(ClampMin=10, Units="cm"))
	float Width = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar", meta=(ClampMin=1, Units="cm"))
	float Height = 10.0f;

	/** Frame thickness around the fill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar", meta=(ClampMin=0, Units="cm"))
	float Border = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar")
	FLinearColor FillColor = FLinearColor(0.85f, 0.05f, 0.02f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar")
	FLinearColor TrailColor = FLinearColor(1.0f, 0.85f, 0.6f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar")
	FLinearColor FrameColor = FLinearColor(0.01f, 0.01f, 0.01f, 1.0f);

	/** Show the bar even when HP is full */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar")
	bool bShowWhenFull = false;

	/** Seconds before the trail strip starts draining */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar", meta=(ClampMin=0, Units="s"))
	float TrailDelay = 0.4f;

	/** Fraction of the bar the trail drains per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HP Bar", meta=(ClampMin=0.1))
	float TrailSpeed = 1.2f;

	/** Health to show (found on the owner automatically if empty) */
	UFUNCTION(BlueprintCallable, Category="HP Bar")
	void SetHealth(UDCHealthComponent* InHealth);

	UFUNCTION(BlueprintPure, Category="HP Bar")
	bool IsBarVisible() const { return bBarVisible; }

	UFUNCTION(BlueprintPure, Category="HP Bar")
	float GetShownPercent() const { return ShownPercent; }

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION()
	void HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser);

	UFUNCTION()
	void HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser);

private:

	UStaticMeshComponent* MakeStrip(const TCHAR* Name, const FLinearColor& Color, float Depth);
	void Layout();
	void SetBarVisible(bool bShow);

	UPROPERTY()
	TObjectPtr<UStaticMesh> BarMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BarMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FrameStrip;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TrailStrip;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FillStrip;

	UPROPERTY()
	TObjectPtr<UDCHealthComponent> Health;

	float ShownPercent = 1.0f;   // fill
	float TrailPercent = 1.0f;   // light strip behind the fill
	float TrailWait = 0.0f;
	bool bBarVisible = true;
};
