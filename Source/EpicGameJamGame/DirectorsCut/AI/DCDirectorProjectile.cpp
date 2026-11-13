// Director's Cut - projectile thrown by the Director boss in Beat 5.

#include "DirectorsCut/AI/DCDirectorProjectile.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ADCDirectorProjectile::ADCDirectorProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(26.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual Mesh"));
	VisualMesh->SetupAttachment(CollisionSphere);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.18f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Landmark.MI_LD_Landmark"));
	if (Cube.Succeeded())
	{
		VisualMesh->SetStaticMesh(Cube.Object);
	}
	if (RedMat.Succeeded())
	{
		VisualMesh->SetMaterial(0, RedMat.Object);
	}

	GlowLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow Light"));
	GlowLight->SetupAttachment(CollisionSphere);
	GlowLight->SetLightColor(FLinearColor(1.0f, 0.15f, 0.1f));
	GlowLight->SetIntensity(2000.0f);
	GlowLight->SetAttenuationRadius(300.0f);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement"));
	ProjectileMovement->InitialSpeed = 2200.0f;
	ProjectileMovement->MaxSpeed = 2200.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 4.0f;
}

void ADCDirectorProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (GetInstigator())
	{
		CollisionSphere->IgnoreActorWhenMoving(GetInstigator(), true);
	}
	CollisionSphere->OnComponentHit.AddUniqueDynamic(this, &ADCDirectorProjectile::HandleHit);
	CollisionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADCDirectorProjectile::HandleOverlap);
}

void ADCDirectorProjectile::HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bSpent || OtherActor == GetInstigator())
	{
		return;
	}
	bSpent = true;
	if (APawn* Pawn = Cast<APawn>(OtherActor); Pawn && Pawn->IsPlayerControlled())
	{
		UGameplayStatics::ApplyDamage(Pawn, Damage, GetInstigatorController(), this, nullptr);
	}
	Destroy();
}

void ADCDirectorProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bSpent || OtherActor == GetInstigator())
	{
		return;
	}
	if (APawn* Pawn = Cast<APawn>(OtherActor); Pawn && Pawn->IsPlayerControlled())
	{
		bSpent = true;
		UGameplayStatics::ApplyDamage(Pawn, Damage, GetInstigatorController(), this, nullptr);
		Destroy();
	}
}
