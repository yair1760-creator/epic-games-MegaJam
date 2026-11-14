// Director's Cut - hand-drawn set dressing.

#include "DirectorsCut/Rendering/DCSetProps.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ADCSetProp::ADCSetProp()
{
	PrimaryActorTick.bCanEverTick = false;   // props don't spin; the sprite component turns itself to the camera

	Blocker = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Blocker"));
	Blocker->SetupAttachment(GetRootComponent());
	Blocker->InitCapsuleSize(60.0f, 80.0f);
	Blocker->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	Blocker->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Blocker->SetHiddenInGame(true);
	Blocker->SetCanEverAffectNavigation(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // the Blocker capsule handles collision
	Mesh->SetCanEverAffectNavigation(false);

	// no placeholder boar picture on props: children set their own art
	Sprite->FrontTexture = nullptr;
	Sprite->SideTexture = nullptr;
	Sprite->bPivotAtBottom = true;
}

void ADCSetProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Blocker->SetCollisionEnabled(bBlocksPawns ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	ApplyLook();
}

void ADCSetProp::ApplyLook()
{
	const bool b3D = bUse3DModel && Mesh && Mesh->GetStaticMesh();
	if (Mesh)
	{
		Mesh->SetVisibility(b3D);
	}
	Sprite->SetVisibility(!b3D, true);
	Sprite->SetComponentTickEnabled(!b3D);
}

void ADCSetProp::BeginPlay()
{
	// a little size variety, decided once per placed copy (stable per actor name so it doesn't change every play)
	if (RandomScale.X != 1.0f || RandomScale.Y != 1.0f)
	{
		const FRandomStream Stream(GetTypeHash(GetFName()));
		const float S = Stream.FRandRange(FMath::Min(RandomScale.X, RandomScale.Y), FMath::Max(RandomScale.X, RandomScale.Y));
		Sprite->SetRelativeScale3D(Sprite->GetRelativeScale3D() * S);
		Mesh->SetRelativeScale3D(Mesh->GetRelativeScale3D() * S);
	}
	Super::BeginPlay();
	ApplyLook();   // after the sprite component's own BeginPlay, which turns its card on
}

// ---------------------------------------------------------------- ready-made props

ADCProp_Grass01::ADCProp_Grass01()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Model(TEXT("/Game/DirectorsCut/Environment/Props/3D/SM_DC_Grass01_3D.SM_DC_Grass01_3D"));
	if (Model.Succeeded())
	{
		Mesh->SetStaticMesh(Model.Object);
	}
	static ConstructorHelpers::FObjectFinder<UTexture2D> Art(TEXT("/Game/DirectorsCut/Environment/Props/T_DC_Prop_Grass01.T_DC_Prop_Grass01"));
	if (Art.Succeeded())
	{
		Sprite->FrontTexture = Art.Object;
	}
	Sprite->FrontSize = FVector2D(140.0f, 121.0f);              // drawing 1604 x 1384
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -2.0f));    // the drawn wooden base sits on the ground
	bBlocksPawns = false;
	RandomScale = FVector2D(0.85f, 1.15f);
}

ADCProp_HayBale01::ADCProp_HayBale01()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Model(TEXT("/Game/DirectorsCut/Environment/Props/3D/SM_DC_HayBale01_3D.SM_DC_HayBale01_3D"));
	if (Model.Succeeded())
	{
		Mesh->SetStaticMesh(Model.Object);
	}
	static ConstructorHelpers::FObjectFinder<UTexture2D> Art(TEXT("/Game/DirectorsCut/Environment/Props/T_DC_Prop_HayBale01.T_DC_Prop_HayBale01"));
	if (Art.Succeeded())
	{
		Sprite->FrontTexture = Art.Object;
	}
	Sprite->FrontSize = FVector2D(300.0f, 208.0f);              // drawing 1492 x 1035
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -6.0f));
	bBlocksPawns = true;
	Blocker->InitCapsuleSize(115.0f, 85.0f);
	Blocker->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
}

ADCProp_AppleTree02::ADCProp_AppleTree02()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Model(TEXT("/Game/DirectorsCut/Environment/Props/3D/SM_DC_AppleTree02_3D.SM_DC_AppleTree02_3D"));
	if (Model.Succeeded())
	{
		Mesh->SetStaticMesh(Model.Object);
	}
	static ConstructorHelpers::FObjectFinder<UTexture2D> Art(TEXT("/Game/DirectorsCut/Environment/Props/T_DC_Prop_AppleTree02.T_DC_Prop_AppleTree02"));
	if (Art.Succeeded())
	{
		Sprite->FrontTexture = Art.Object;
	}
	Sprite->FrontSize = FVector2D(525.0f, 550.0f);              // drawing 1467 x 1536
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));   // the drawn stand is ~8% above the drawing's bottom edge
	bBlocksPawns = true;
	Blocker->InitCapsuleSize(45.0f, 160.0f);                   // trunk only
	Blocker->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	RandomScale = FVector2D(0.9f, 1.1f);
}
