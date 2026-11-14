// Director's Cut - Beat 5 final boss: The Director enters the scene.

#include "DirectorsCut/AI/DCDirectorBoss.h"
#include "DirectorsCut/AI/DCDirectorProjectile.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "DirectorsCut/Combat/DCFloatingHPBar.h"
#include "DirectorsCut/Combat/DCFXLibrary.h"
#include "DirectorsCut/Rendering/DCSpriteFX.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "Quest/DCQuestSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ADCDirectorBoss::ADCDirectorBoss()
{
	DC_DEFAULT_SFX(AttackSound, "S_BossShot");
	DC_DEFAULT_SFX(HitSound, "S_BossHit");
	DC_DEFAULT_SFX(DefeatSound, "S_BossDefeat");
	DefeatSpriteFX = ADCFX_BossDefeat::StaticClass();
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(65.0f, 130.0f);

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = 420.0f;
	Move->GravityScale = 1.0f;

	Health = CreateDefaultSubobject<UDCHealthComponent>(TEXT("Health"));
	Health->MaxHP = 450.0f;   // sword (25 dmg) needs 18 hits
	Health->bInvulnerable = true;   // unlocked when StartFight() runs after his intro monologue

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Game/LevelPrototyping/Meshes/SM_Cylinder.SM_Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DarkMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Dark.MI_LD_Dark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WhiteMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_White.MI_LD_White"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Landmark.MI_LD_Landmark"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMat(TEXT("/Game/VerticalSlice/Materials/MI_LD_Breadcrumb.MI_LD_Breadcrumb"));

	CoatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Coat Mesh"));
	CoatMesh->SetupAttachment(GetCapsuleComponent());
	CoatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoatMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
	CoatMesh->SetRelativeScale3D(FVector(0.85f, 0.95f, 1.65f));
	if (Cube.Succeeded()) CoatMesh->SetStaticMesh(Cube.Object);
	if (DarkMat.Succeeded()) CoatMesh->SetMaterial(0, DarkMat.Object);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head Mesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 92.0f));
	HeadMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.55f));
	if (Cube.Succeeded()) HeadMesh->SetStaticMesh(Cube.Object);
	if (WhiteMat.Succeeded()) HeadMesh->SetMaterial(0, WhiteMat.Object);

	BeretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beret Mesh"));
	BeretMesh->SetupAttachment(HeadMesh);
	BeretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeretMesh->SetRelativeLocation(FVector(-12.0f, 0.0f, 52.0f));
	BeretMesh->SetRelativeRotation(FRotator(-12.0f, 0.0f, 0.0f));
	BeretMesh->SetRelativeScale3D(FVector(1.35f, 1.35f, 0.35f));
	if (Cyl.Succeeded()) BeretMesh->SetStaticMesh(Cyl.Object);
	if (RedMat.Succeeded()) BeretMesh->SetMaterial(0, RedMat.Object);

	MegaphoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Megaphone Mesh"));
	MegaphoneMesh->SetupAttachment(GetCapsuleComponent());
	MegaphoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MegaphoneMesh->SetRelativeLocation(FVector(65.0f, 38.0f, 45.0f));
	MegaphoneMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	MegaphoneMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.7f));
	if (Cyl.Succeeded()) MegaphoneMesh->SetStaticMesh(Cyl.Object);
	if (GoldMat.Succeeded()) MegaphoneMesh->SetMaterial(0, GoldMat.Object);

	SpotlightAura = CreateDefaultSubobject<UPointLightComponent>(TEXT("Spotlight Aura"));
	SpotlightAura->SetupAttachment(GetCapsuleComponent());
	SpotlightAura->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	SpotlightAura->SetLightColor(FLinearColor(1.0f, 0.2f, 0.1f));
	SpotlightAura->SetIntensity(6000.0f);
	SpotlightAura->SetAttenuationRadius(900.0f);

	HPBar = CreateDefaultSubobject<UDCFloatingHPBar>(TEXT("HP Bar"));
	HPBar->SetupAttachment(GetCapsuleComponent());
	HPBar->SetRelativeLocation(FVector(0.0f, 0.0f, 175.0f));
	HPBar->bShowWhenFull = true;

	NameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Name Text"));
	NameText->SetupAttachment(GetCapsuleComponent());
	NameText->SetRelativeLocation(FVector(0.0f, 0.0f, 215.0f));
	NameText->SetHorizontalAlignment(EHTA_Center);
	NameText->SetWorldSize(44.0f);
	NameText->SetText(FText::FromString(TEXT("THE DIRECTOR")));
	NameText->SetTextRenderColor(FColor::Red);
	NameText->SetUsingAbsoluteRotation(true);

	Tags.Add(TEXT("DirectorBoss"));
}

void ADCDirectorBoss::BeginPlay()
{
	Super::BeginPlay();

	Health->OnHealthChanged.AddUniqueDynamic(this, &ADCDirectorBoss::HandleHealthChanged);
	Health->OnDied.AddUniqueDynamic(this, &ADCDirectorBoss::HandleDied);

	const FVector Delta = GetActorLocation() - ArenaCenter;
	OrbitAngle = FMath::Atan2(Delta.Y, Delta.X);
}

void ADCDirectorBoss::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FireTimer);
	GetWorldTimerManager().ClearTimer(TauntTimer);
	Super::EndPlay(EndPlayReason);
}

void ADCDirectorBoss::StartFight()
{
	if (bDead || bFightActive)
	{
		return;
	}
	bFightActive = true;
	Health->bInvulnerable = false;

	GetWorldTimerManager().SetTimer(FireTimer, this, &ADCDirectorBoss::FireAtPlayer, FireInterval, true, 0.6f);
	GetWorldTimerManager().SetTimer(TauntTimer, this, &ADCDirectorBoss::TriggerTaunt, TauntInterval, true, 5.0f);
}

void ADCDirectorBoss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (NameText && Player)
	{
		NameText->SetWorldRotation(FRotator(0.0f, (Player->GetActorLocation() - NameText->GetComponentLocation()).Rotation().Yaw, 0.0f));
	}

	if (bDead)
	{
		return;
	}

	if (Player)
	{
		const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
		SetActorRotation(FRotator(0.0f, ToPlayer.Rotation().Yaw, 0.0f));
	}

	if (bFightActive)
	{
		// Strafe around The Set arena so the fight stays dynamic
		const float AngularSpeed = (Health->GetHPPercent() <= 0.5f ? 0.65f : 0.42f) * OrbitDir;
		OrbitAngle += AngularSpeed * DeltaSeconds;
		const float Radius = 1350.0f;
		const FVector DesiredXY(ArenaCenter.X + Radius * FMath::Cos(OrbitAngle), ArenaCenter.Y + Radius * FMath::Sin(OrbitAngle), GetActorLocation().Z);
		const FVector MoveDir = (DesiredXY - GetActorLocation()).GetSafeNormal2D();
		AddMovementInput(MoveDir, 1.0f);
	}
}

void ADCDirectorBoss::FireAtPlayer()
{
	if (!IsFightActive())
	{
		return;
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	const FVector TargetLoc = Player->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	UDCFXLibrary::PlayWorldSound(this, AttackSound, GetActorLocation(), 1.4f);
	SpawnProjectileTowards(TargetLoc, 0.0f);

	// Enraged phase under 50% HP: 3-shot dramatic fan + flip strafe direction occasionally
	if (Health->GetHPPercent() <= 0.5f)
	{
		SpawnProjectileTowards(TargetLoc, -14.0f);
		SpawnProjectileTowards(TargetLoc, 14.0f);
		if (FMath::FRand() < 0.35f)
		{
			OrbitDir = -OrbitDir;
		}
	}
}

void ADCDirectorBoss::SpawnProjectileTowards(const FVector& TargetPoint, float YawOffsetDeg)
{
	const FVector MuzzleLoc = GetActorLocation() + GetActorForwardVector() * 110.0f + FVector(0.0f, 0.0f, 45.0f);
	FRotator AimRot = (TargetPoint - MuzzleLoc).Rotation();
	AimRot.Yaw += YawOffsetDeg;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<ADCDirectorProjectile>(ADCDirectorProjectile::StaticClass(), MuzzleLoc, AimRot, Params);
}

void ADCDirectorBoss::TriggerTaunt()
{
	if (IsFightActive())
	{
		OnBossTaunt.Broadcast();
	}
}

void ADCDirectorBoss::HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser)
{
	if (Delta < 0.0f && !HealthComponent->IsDead())
	{
		UDCFXLibrary::PlayWorldSound(this, HitSound, GetActorLocation(), 1.2f, 0.08f, 0.15f);
	}
	if (!bHalfHPTriggered && Delta < 0.0f && HealthComponent->GetHPPercent() <= 0.5f && !HealthComponent->IsDead())
	{
		bHalfHPTriggered = true;
		OnBossHalfHP.Broadcast();
	}
}

void ADCDirectorBoss::HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	bFightActive = false;

	GetWorldTimerManager().ClearTimer(FireTimer);
	GetWorldTimerManager().ClearTimer(TauntTimer);

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tags.Add(TEXT("Dead"));

	if (NameText)
	{
		NameText->SetText(FText::FromString(TEXT("THAT'S A WRAP!")));
		NameText->SetTextRenderColor(FColor::Green);
	}

	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReportEvent(FName("Kill.Director"));
	}

	UDCFXLibrary::PlayWorldSound(this, DefeatSound, GetActorLocation(), 1.6f, 0.0f, 0.0f);
	UDCFXLibrary::SpawnWorldFX(this, DefeatFX, GetActorLocation());
	UDCFXLibrary::SpawnSpriteFX(this, DefeatSpriteFX, GetActorLocation(), 1.0f, 60.0f);

	OnBossDied.Broadcast();
	SetLifeSpan(5.0f);
}
