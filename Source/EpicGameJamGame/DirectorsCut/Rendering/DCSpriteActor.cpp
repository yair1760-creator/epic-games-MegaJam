// Director's Cut - standalone sprite actor.

#include "DirectorsCut/Rendering/DCSpriteActor.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ADCSpriteActor::ADCSpriteActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Sprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Sprite"));
	Sprite->SetupAttachment(GetRootComponent());

	// placeholder boar until real art is in
	static ConstructorHelpers::FObjectFinder<UTexture2D> Front(TEXT("/Game/DirectorsCut/Rendering/Sprites/T_DC_Boar_Front_Placeholder.T_DC_Boar_Front_Placeholder"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Side(TEXT("/Game/DirectorsCut/Rendering/Sprites/T_DC_Boar_Side_Placeholder.T_DC_Boar_Side_Placeholder"));
	if (Front.Succeeded()) Sprite->FrontTexture = Front.Object;
	if (Side.Succeeded())  Sprite->SideTexture = Side.Object;
}

void ADCSpriteActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (SpinSpeed != 0.0f)
	{
		AddActorWorldRotation(FRotator(0.0f, SpinSpeed * DeltaSeconds, 0.0f));
	}
}
