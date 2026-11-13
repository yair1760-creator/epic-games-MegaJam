// Director's Cut - floating HP bar.

#include "DirectorsCut/Combat/DCFloatingHPBar.h"
#include "DirectorsCut/Combat/DCHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UDCFloatingHPBar::UDCFloatingHPBar()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Flat(TEXT("/Game/DirectorsCut/Rendering/Materials/M_DC_FlatUnlit.M_DC_FlatUnlit"));
	if (Cube.Succeeded())
	{
		BarMesh = Cube.Object;
	}
	if (Flat.Succeeded())
	{
		BarMaterial = Flat.Object;
	}
}

UStaticMeshComponent* UDCFloatingHPBar::MakeStrip(const TCHAR* Name, const FLinearColor& Color, float Depth)
{
	UStaticMeshComponent* Strip = NewObject<UStaticMeshComponent>(GetOwner(), Name);
	Strip->SetStaticMesh(BarMesh);
	Strip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Strip->SetCastShadow(false);
	Strip->SetupAttachment(this);
	Strip->SetRelativeLocation(FVector(Depth, 0.0f, 0.0f));
	Strip->RegisterComponent();

	if (BarMaterial)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BarMaterial, Strip);
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		Strip->SetMaterial(0, MID);
	}
	return Strip;
}

void UDCFloatingHPBar::BeginPlay()
{
	Super::BeginPlay();

	// cube is 100 cm with a centred pivot; X is the bar's thickness and points at the camera
	FrameStrip = MakeStrip(TEXT("HPBar_Frame"), FrameColor, 0.0f);
	TrailStrip = MakeStrip(TEXT("HPBar_Trail"), TrailColor, 0.6f);
	FillStrip = MakeStrip(TEXT("HPBar_Fill"), FillColor, 1.2f);

	if (!Health)
	{
		SetHealth(UDCHealthComponent::FindHealth(GetOwner()));
	}

	ShownPercent = TrailPercent = Health ? Health->GetHPPercent() : 1.0f;
	Layout();
	SetBarVisible(bShowWhenFull || ShownPercent < 1.0f);
}

void UDCFloatingHPBar::SetHealth(UDCHealthComponent* InHealth)
{
	if (Health)
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UDCFloatingHPBar::HandleHealthChanged);
		Health->OnDied.RemoveDynamic(this, &UDCFloatingHPBar::HandleDied);
	}
	Health = InHealth;
	if (Health)
	{
		Health->OnHealthChanged.AddUniqueDynamic(this, &UDCFloatingHPBar::HandleHealthChanged);
		Health->OnDied.AddUniqueDynamic(this, &UDCFloatingHPBar::HandleDied);
	}
}

void UDCFloatingHPBar::HandleHealthChanged(UDCHealthComponent* HealthComponent, float CurrentHP, float Delta, AActor* DamageCauser)
{
	ShownPercent = HealthComponent->GetHPPercent();
	if (Delta < 0.0f)
	{
		TrailWait = TrailDelay;            // let the trail show the chunk that was lost
	}
	else
	{
		TrailPercent = ShownPercent;       // healing: no trail
	}
	Layout();
	SetBarVisible(!HealthComponent->IsDead() && (bShowWhenFull || ShownPercent < 1.0f));
}

void UDCFloatingHPBar::HandleDied(UDCHealthComponent* HealthComponent, AController* Killer, AActor* DamageCauser)
{
	SetBarVisible(false);
}

void UDCFloatingHPBar::SetBarVisible(bool bShow)
{
	bBarVisible = bShow;
	SetVisibility(bShow, /*bPropagateToChildren*/ true);
	Layout();
}

void UDCFloatingHPBar::Layout()
{
	if (!FillStrip)
	{
		return;
	}
	const float InnerW = FMath::Max(Width - 2.0f * Border, 1.0f);
	const float InnerH = FMath::Max(Height - 2.0f * Border, 1.0f);

	FrameStrip->SetRelativeScale3D(FVector(0.01f, Width / 100.0f, Height / 100.0f));

	// seen from the camera, +Y is screen-left: anchor the strips on the left so they drain to the left
	auto Place = [&](UStaticMeshComponent* Strip, float Percent, float Depth)
	{
		const float P = FMath::Clamp(Percent, 0.0f, 1.0f);
		const float W = FMath::Max(InnerW * P, 0.01f);
		Strip->SetRelativeScale3D(FVector(0.01f, W / 100.0f, InnerH / 100.0f));
		Strip->SetRelativeLocation(FVector(Depth, (InnerW - W) * 0.5f, 0.0f));
		Strip->SetVisibility(bBarVisible && P > 0.0f);
	};
	Place(TrailStrip, TrailPercent, 0.6f);
	Place(FillStrip, ShownPercent, 1.2f);
}

void UDCFloatingHPBar::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bBarVisible)
	{
		return;
	}

	// face the player camera
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		// the pawn's eyes are current this frame; the camera manager's cached view can lag a frame
		FVector CamLoc;
		FRotator CamRot;
		if (const APawn* ViewPawn = PC->GetPawn())
		{
			CamLoc = ViewPawn->GetPawnViewLocation();
		}
		else
		{
			PC->GetPlayerViewPoint(CamLoc, CamRot);
		}
		SetWorldRotation((CamLoc - GetComponentLocation()).Rotation());
	}

	// drain the trail toward the fill
	if (TrailPercent > ShownPercent)
	{
		if (TrailWait > 0.0f)
		{
			TrailWait -= DeltaTime;
		}
		else
		{
			TrailPercent = FMath::Max(ShownPercent, TrailPercent - TrailSpeed * DeltaTime);
			Layout();
		}
	}
}
