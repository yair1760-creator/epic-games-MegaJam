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
	DC_FX_FRAME(F0, "T_FX_HitSplat_0"); DC_FX_FRAME(F1, "T_FX_HitSplat_1");
	DC_FX_FRAME(F2, "T_FX_HitSplat_2"); DC_FX_FRAME(F3, "T_FX_HitSplat_3");
	for (UTexture2D* T : { F0.Object, F1.Object, F2.Object, F3.Object })
	{
		if (T) Frames.Add(T);
	}
	FPS = 16.0f;
	Size = FVector2D(70.0f, 70.0f);
}

ADCFX_BoarPoof::ADCFX_BoarPoof()
{
	DC_FX_FRAME(F0, "T_FX_Poof_0"); DC_FX_FRAME(F1, "T_FX_Poof_1"); DC_FX_FRAME(F2, "T_FX_Poof_2");
	DC_FX_FRAME(F3, "T_FX_Poof_3"); DC_FX_FRAME(F4, "T_FX_Poof_4");
	for (UTexture2D* T : { F0.Object, F1.Object, F2.Object, F3.Object, F4.Object })
	{
		if (T) Frames.Add(T);
	}
	FPS = 12.0f;
	Size = FVector2D(150.0f, 150.0f);
}

ADCFX_BossDefeat::ADCFX_BossDefeat()
{
	DC_FX_FRAME(F0, "T_FX_Poof_0"); DC_FX_FRAME(F1, "T_FX_Poof_1"); DC_FX_FRAME(F2, "T_FX_Poof_2");
	DC_FX_FRAME(F3, "T_FX_Poof_3"); DC_FX_FRAME(F4, "T_FX_Poof_4");
	for (UTexture2D* T : { F0.Object, F1.Object, F2.Object, F3.Object, F4.Object })
	{
		if (T) Frames.Add(T);
	}
	FPS = 9.0f;
	Size = FVector2D(420.0f, 420.0f);
	Tint = FLinearColor(1.0f, 0.82f, 0.35f);   // gold
	Brightness = 1.5f;
}

#undef DC_FX_FRAME
