// Director's Cut - one-shot sprite effects.

#include "DirectorsCut/Rendering/DCSpriteFX.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

ADCSpriteFX::ADCSpriteFX()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Sprite"));
	Sprite->SetupAttachment(Root);
	Sprite->bPivotAtBottom = false;   // centered on the hit point
	Sprite->bCastShadow = false;      // a flash of light / dust has no shadow
}

void ADCSpriteFX::BeginPlay()
{
	Sprite->FrontSize = Size;
	Sprite->Tint = Tint;
	Sprite->Brightness = Brightness;
	if (Frames.Num() > 0)
	{
		FDCSpriteAnim Anim;
		for (UTexture2D* T : Frames)
		{
			if (T)
			{
				Anim.Frames.Add(T);
			}
		}
		Anim.FPS = FPS;
		Anim.bLoop = false;
		Sprite->FrontTexture = Anim.Frames.Num() ? Anim.Frames[0].Get() : nullptr;
		Sprite->Animations.Add(TEXT("FX"), Anim);
		Sprite->StartAnimation = TEXT("FX");
	}

	Super::BeginPlay();   // the sprite component builds its card and starts the animation here

	// gone right after the last frame has had its turn
	SetLifeSpan(FMath::Max(Frames.Num(), 1) / FMath::Max(FPS, 1.0f) + 0.01f);
}

// ---------------------------------------------------------------- ready-made effects

#define DC_FX_FRAME(Var, Name) static ConstructorHelpers::FObjectFinder<UTexture2D> Var(TEXT("/Game/DirectorsCut/FX/Textures/" Name "." Name))

ADCFX_HitSplat::ADCFX_HitSplat()
{
	// Taisho's stars puff: small burst -> full stars and ring -> breaking up (his file numbers them the other way round)
	DC_FX_FRAME(F0, "T_FX_StarsPuff_04"); DC_FX_FRAME(F1, "T_FX_StarsPuff_03");
	DC_FX_FRAME(F2, "T_FX_StarsPuff_02"); DC_FX_FRAME(F3, "T_FX_StarsPuff_01");
	for (UTexture2D* T : { F0.Object, F1.Object, F2.Object, F3.Object })
	{
		if (T) Frames.Add(T);
	}
	FPS = 14.0f;
	Size = FVector2D(100.0f, 87.0f);          // the frames are 603 x 527
}

// Taisho's puff of smoke, 11 frames: pop -> big cloud -> wisps (frames 958 x 474)
#define DC_SMOKE_FRAMES 	DC_FX_FRAME(F01, "T_FX_SmokePuff_01"); DC_FX_FRAME(F02, "T_FX_SmokePuff_02"); DC_FX_FRAME(F03, "T_FX_SmokePuff_03"); 	DC_FX_FRAME(F04, "T_FX_SmokePuff_04"); DC_FX_FRAME(F05, "T_FX_SmokePuff_05"); DC_FX_FRAME(F06, "T_FX_SmokePuff_06"); 	DC_FX_FRAME(F07, "T_FX_SmokePuff_07"); DC_FX_FRAME(F08, "T_FX_SmokePuff_08"); DC_FX_FRAME(F09, "T_FX_SmokePuff_09"); 	DC_FX_FRAME(F10, "T_FX_SmokePuff_10"); DC_FX_FRAME(F11, "T_FX_SmokePuff_11"); 	for (UTexture2D* T : { F01.Object, F02.Object, F03.Object, F04.Object, F05.Object, F06.Object, F07.Object, F08.Object, F09.Object, F10.Object, F11.Object }) 	{ 		if (T) Frames.Add(T); 	}

ADCFX_BoarPoof::ADCFX_BoarPoof()
{
	DC_SMOKE_FRAMES
	FPS = 16.0f;
	Size = FVector2D(200.0f, 99.0f);
}

ADCFX_BossDefeat::ADCFX_BossDefeat()
{
	DC_SMOKE_FRAMES
	FPS = 12.0f;
	Size = FVector2D(460.0f, 228.0f);
	Tint = FLinearColor(1.0f, 0.82f, 0.35f);   // gold
	Brightness = 1.5f;
}

#undef DC_SMOKE_FRAMES
#undef DC_FX_FRAME
