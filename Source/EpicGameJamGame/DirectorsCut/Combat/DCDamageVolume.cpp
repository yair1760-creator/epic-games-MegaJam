// Director's Cut - damage volume.

#include "DirectorsCut/Combat/DCDamageVolume.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

ADCDamageVolume::ADCDamageVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
	Box->SetBoxExtent(FVector(300.0f, 300.0f, 100.0f));
	Box->SetCollisionProfileName(TEXT("Trigger"));
	Box->SetGenerateOverlapEvents(true);
	Box->ShapeColor = FColor::Red;
}

void ADCDamageVolume::BeginPlay()
{
	Super::BeginPlay();

	Box->OnComponentBeginOverlap.AddDynamic(this, &ADCDamageVolume::OnBoxBeginOverlap);
	GetWorldTimerManager().SetTimer(DamageTimer, this, &ADCDamageVolume::DamageOverlapping, TickInterval, true);
}

void ADCDamageVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageTimer);
	Super::EndPlay(EndPlayReason);
}

void ADCDamageVolume::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	HurtActor(OtherActor);
}

void ADCDamageVolume::DamageOverlapping()
{
	TArray<AActor*> Inside;
	Box->GetOverlappingActors(Inside, APawn::StaticClass());
	for (AActor* Actor : Inside)
	{
		HurtActor(Actor);
	}
}

void ADCDamageVolume::HurtActor(AActor* Actor)
{
	APawn* Pawn = Cast<APawn>(Actor);
	if (!bEnabled || !Pawn || DamagePerTick <= 0.0f)
	{
		return;
	}
	if (bPlayersOnly && !Pawn->IsPlayerControlled())
	{
		return;
	}
	UGameplayStatics::ApplyDamage(Pawn, DamagePerTick, nullptr, this, nullptr);
}
