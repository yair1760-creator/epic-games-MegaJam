// Director's Cut - training dummy for testing damage.

#include "DirectorsCut/Combat/DCTrainingDummy.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ADCTrainingDummy::ADCTrainingDummy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;   // keeps the HP text turned to the camera

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	if (Cylinder.Succeeded())
	{
		Mesh->SetStaticMesh(Cylinder.Object);
		Mesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.8f));   // d70 x h180 cm, pivot at the bottom
	}

	HPText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HPText"));
	HPText->SetupAttachment(Mesh);
	HPText->SetAbsolute(false, false, true);                    // world scale, so the mesh scale doesn't stretch the text
	HPText->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));   // relative to the scaled mesh: ~2.2 m up
	HPText->SetHorizontalAlignment(EHTA_Center);
	HPText->SetWorldSize(36.0f);
	HPText->SetTextRenderColor(FColor::White);

	Health = CreateDefaultSubobject<UDCHealthComponent>(TEXT("Health"));

	Tags.Add(TEXT("TestDummy"));
}

void ADCTrainingDummy::BeginPlay()
{
	Super::BeginPlay();

	BaseScale = Mesh->GetRelativeScale3D();
	BaseRotation = GetActorRotation();

	Health->OnHealthChanged.AddDynamic(this, &ADCTrainingDummy::OnHealthChanged);
	Health->OnDied.AddDynamic(this, &ADCTrainingDummy::OnDied);
	UpdateText();
}

void ADCTrainingDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// face the text to the player camera
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		FVector CamLoc;
		FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		const FVector ToCam = CamLoc - HPText->GetComponentLocation();
		HPText->SetWorldRotation(FRotator(0.0f, ToCam.Rotation().Yaw, 0.0f));
	}

	// hit pop: quick squash that eases back
	if (PopAlpha > 0.0f)
	{
		PopAlpha = FMath::Max(0.0f, PopAlpha - DeltaSeconds * 6.0f);
		const float S = HitPopScale * PopAlpha;
		Mesh->SetRelativeScale3D(BaseScale * FVector(1.0f + S, 1.0f + S, 1.0f - S));
	}
}

void ADCTrainingDummy::OnHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser)
{
	if (Delta < 0.0f)
	{
		PopAlpha = 1.0f;
	}
	UpdateText();
}

void ADCTrainingDummy::OnDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser)
{
	// tip over and stop blocking
	SetActorRotation(BaseRotation + FRotator(0.0f, 0.0f, 80.0f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HPText->SetText(FText::FromString(TEXT("KO")));
	HPText->SetTextRenderColor(FColor::Red);

	if (RespawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ADCTrainingDummy::StandUp, RespawnDelay, false);
	}
}

void ADCTrainingDummy::StandUp()
{
	SetActorRotation(BaseRotation);
	Mesh->SetRelativeScale3D(BaseScale);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Health->ResetHealth();
	HPText->SetTextRenderColor(FColor::White);
	UpdateText();
}

void ADCTrainingDummy::UpdateText()
{
	if (!Health->IsDead())
	{
		HPText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), Health->GetCurrentHP(), Health->GetMaxHP())));
		const float P = Health->GetHPPercent();
		HPText->SetTextRenderColor(P > 0.5f ? FColor::White : (P > 0.25f ? FColor::Yellow : FColor(255, 120, 0)));
	}
}
