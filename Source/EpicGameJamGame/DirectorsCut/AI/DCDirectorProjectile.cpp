// Director's Cut - projectile thrown by the Director boss in Beat 5.

#include "DirectorsCut/AI/DCDirectorProjectile.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Engine/Texture2D.h"
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

	Sprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Sprite"));
	Sprite->SetupAttachment(CollisionSphere);
	Sprite->bPivotAtBottom = false;
	Sprite->bCastShadow = false;
	Sprite->FrontSize = FVector2D(78.0f, 78.0f);
	Sprite->SideSize = Sprite->FrontSize;
	#define DC_CLAPPER_FRAME(Var, N) static ConstructorHelpers::FObjectFinder<UTexture2D> Var(TEXT("/Game/DirectorsCut/Characters/Director/T_Director_Clapper_" N ".T_Director_Clapper_" N))
	DC_CLAPPER_FRAME(C1, "01");
	DC_CLAPPER_FRAME(C2, "02");
	DC_CLAPPER_FRAME(C3, "03");
	DC_CLAPPER_FRAME(C4, "04");
	#undef DC_CLAPPER_FRAME
	if (C1.Succeeded() && C2.Succeeded() && C3.Succeeded() && C4.Succeeded())
	{
		FDCSpriteAnim Spin;
		Spin.Frames = { C1.Object, C2.Object, C3.Object, C4.Object };
		Spin.FPS = 14.0f;
		Spin.bLoop = true;
		Sprite->Animations.Add(TEXT("Spin"), Spin);
		Sprite->StartAnimation = TEXT("Spin");
		Sprite->FrontTexture = C1.Object;
		VisualMesh->SetVisibility(false);
	}
	else
	{
		Sprite->SetVisibility(false, true);
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
