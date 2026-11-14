// Director's Cut - camera-facing sprite.

#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

// Engine plane: 100x100 cm, normal +Z. This rotation stands it up with its picture readable from +X
// (this component's X axis is turned toward the camera every frame).
static const FRotator QuadToCamera(0.0f, -90.0f, 90.0f);

UDCBillboardSprite::UDCBillboardSprite()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;   // after movement, so the card never lags its owner

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Game/DirectorsCut/Rendering/Materials/M_DC_Sprite.M_DC_Sprite"));
	if (Plane.Succeeded())
	{
		QuadMesh = Plane.Object;
	}
	if (Mat.Succeeded())
	{
		SpriteMaterial = Mat.Object;
	}
}

void UDCBillboardSprite::BeginPlay()
{
	Super::BeginPlay();

	// unique name: an actor can carry several sprites (King Boar body + crown); a fixed name would make the second replace the first
	Quad = NewObject<UStaticMeshComponent>(GetOwner(), MakeUniqueObjectName(GetOwner(), UStaticMeshComponent::StaticClass(), TEXT("SpriteQuad")));
	Quad->SetStaticMesh(QuadMesh);
	Quad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Quad->SetCastShadow(bCastShadow);       // cards cast a (flat) shadow, which grounds them (effects turn it off)
	Quad->bCastHiddenShadow = false;
	Quad->SetupAttachment(this);
	Quad->SetRelativeRotation(QuadToCamera);
	Quad->RegisterComponent();

	if (SpriteMaterial)
	{
		MID = UMaterialInstanceDynamic::Create(SpriteMaterial, Quad);
		Quad->SetMaterial(0, MID);
	}
	SetTint(Tint, Brightness);
	ApplyView(EDCSpriteView::Front, false, /*bForce*/ true);

	if (!StartAnimation.IsNone())
	{
		PlayAnimation(StartAnimation, 1.0f, true);
	}
}

// ---------------------------------------------------------------- frame animation

void UDCBillboardSprite::PlayAnimation(FName Name, float PlayRate, bool bRestart)
{
	AnimRate = FMath::Max(PlayRate, 0.0f);

	// a one-shot in progress keeps playing; remember what to go back to
	if (bOneShotActive && !bRestart)
	{
		OneShotReturn = Name;
		OneShotReturnRate = AnimRate;
		return;
	}
	if (Name == CurrentAnim && !bRestart && !bAnimFinished)
	{
		return;
	}
	if (!Animations.Contains(Name))
	{
		return;
	}

	CurrentAnim = Name;
	FrameTime = 0.0f;
	bAnimFinished = false;
	bOneShotActive = false;
	ShowFrame(0);
}

void UDCBillboardSprite::PlayOneShot(FName Name, FName ThenPlay)
{
	if (!Animations.Contains(Name))
	{
		return;
	}
	OneShotReturn = ThenPlay.IsNone() ? CurrentAnim : ThenPlay;
	OneShotReturnRate = AnimRate;
	CurrentAnim = Name;
	AnimRate = 1.0f;
	FrameTime = 0.0f;
	bAnimFinished = false;
	bOneShotActive = true;
	ShowFrame(0);
}

void UDCBillboardSprite::SetPlayRate(float PlayRate)
{
	if (bOneShotActive)
	{
		OneShotReturnRate = FMath::Max(PlayRate, 0.0f);
	}
	else
	{
		AnimRate = FMath::Max(PlayRate, 0.0f);
	}
}

void UDCBillboardSprite::ShowFrame(int32 Index)
{
	const FDCSpriteAnim* Anim = Animations.Find(CurrentAnim);
	if (!Anim || Anim->Frames.Num() == 0)
	{
		return;
	}
	FrameIndex = FMath::Clamp(Index, 0, Anim->Frames.Num() - 1);
	if (UTexture2D* Frame = Anim->Frames[FrameIndex])
	{
		if (Frame != FrontTexture)
		{
			FrontTexture = Frame;
			ApplyView(CurrentView, bMirrored, /*bForce*/ true);
		}
	}
}

void UDCBillboardSprite::TickAnimation(float DeltaTime)
{
	const FDCSpriteAnim* Anim = Animations.Find(CurrentAnim);
	if (!Anim || Anim->Frames.Num() == 0 || bAnimFinished || AnimRate <= 0.0f)
	{
		return;
	}

	FrameTime += DeltaTime * AnimRate;
	for (int32 Guard = 0; Guard < 8; ++Guard)
	{
		const float Duration = Anim->FrameDurations.IsValidIndex(FrameIndex) && Anim->FrameDurations[FrameIndex] > 0.0f
			? Anim->FrameDurations[FrameIndex] : 1.0f / FMath::Max(Anim->FPS, 0.1f);
		if (FrameTime < Duration)
		{
			break;
		}
		FrameTime -= Duration;

		if (FrameIndex + 1 < Anim->Frames.Num())
		{
			ShowFrame(FrameIndex + 1);
		}
		else if (bOneShotActive)
		{
			// one-shot done: back to what should be playing now
			bOneShotActive = false;
			const FName Next = OneShotReturn;
			OneShotReturn = NAME_None;
			CurrentAnim = NAME_None;
			PlayAnimation(Next, OneShotReturnRate, true);
			return;
		}
		else if (Anim->bLoop)
		{
			ShowFrame(0);
		}
		else
		{
			bAnimFinished = true;   // hold the last frame
			return;
		}
	}
}

void UDCBillboardSprite::SetTextures(UTexture2D* InFront, UTexture2D* InSide, UTexture2D* InBack)
{
	FrontTexture = InFront;
	SideTexture = InSide;
	BackTexture = InBack;
	ApplyView(CurrentView, bMirrored, true);
}

void UDCBillboardSprite::SetTint(FLinearColor InTint, float InBrightness)
{
	Tint = InTint;
	Brightness = InBrightness;
	if (MID)
	{
		MID->SetVectorParameterValue(TEXT("Tint"), Tint);
		MID->SetScalarParameterValue(TEXT("Brightness"), Brightness);
	}
}

UTexture2D* UDCBillboardSprite::TextureFor(EDCSpriteView View) const
{
	UTexture2D* T = nullptr;
	switch (View)
	{
	case EDCSpriteView::Side: T = SideTexture; break;
	case EDCSpriteView::Back: T = BackTexture ? BackTexture.Get() : FrontTexture.Get(); break;
	default:                  T = FrontTexture; break;
	}
	// fall back to whatever we have
	if (!T) T = FrontTexture ? FrontTexture.Get() : SideTexture.Get();
	return T;
}

void UDCBillboardSprite::ApplyView(EDCSpriteView View, bool bMirror, bool bForce)
{
	if (!Quad || (!bForce && View == CurrentView && bMirror == bMirrored))
	{
		return;
	}
	CurrentView = View;
	bMirrored = bMirror;

	// a view without its own picture uses the front size with the fallback picture
	const bool bHasSide = SideTexture != nullptr;
	const FVector2D Size = (View == EDCSpriteView::Side && bHasSide) ? SideSize : FrontSize;
	Quad->SetRelativeScale3D(FVector(Size.X / 100.0f, Size.Y / 100.0f, 1.0f));
	Quad->SetRelativeLocation(FVector(0.0f, 0.0f, bPivotAtBottom ? Size.Y * 0.5f : 0.0f));

	if (MID)
	{
		if (UTexture2D* T = TextureFor(View))
		{
			MID->SetTextureParameterValue(TEXT("SpriteTex"), T);
		}
		MID->SetScalarParameterValue(TEXT("FlipU"), bMirror ? 1.0f : 0.0f);
	}
}

void UDCBillboardSprite::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TickAnimation(DeltaTime);

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !Quad)
	{
		return;
	}

	// where the player looks from (the pawn's eyes are current this frame; the camera cache can lag)
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	if (const APawn* ViewPawn = PC->GetPawn())
	{
		CamLoc = ViewPawn->GetPawnViewLocation();
	}

	FVector ToCam = CamLoc - GetComponentLocation();
	ToCam.Z = 0.0f;
	if (!ToCam.Normalize())
	{
		return;
	}

	// upright card, turned to the camera
	SetWorldRotation(FRotator(0.0f, ToCam.Rotation().Yaw, 0.0f));

	// which picture: compare the owner's facing with the direction to the camera
	const AActor* Owner = GetOwner();
	FVector Facing = Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
	Facing.Z = 0.0f;
	Facing.Normalize();

	const float CosFront = FMath::Cos(FMath::DegreesToRadians(FrontViewAngle));
	const float Dot = FVector::DotProduct(Facing, ToCam);

	if (Dot >= CosFront)
	{
		ApplyView(EDCSpriteView::Front, false);
	}
	else if (Dot <= -CosFront)
	{
		ApplyView(EDCSpriteView::Back, false);
	}
	else
	{
		// is the owner heading to the right side of the player's screen?
		// screen-right when looking at the card = (-ToCam) x Up ... in UE's left-handed frame: Up ^ ViewDir
		const FVector ViewDir = -ToCam;
		const FVector ScreenRight = FVector::CrossProduct(FVector::UpVector, ViewDir);
		const bool bHeadingRight = FVector::DotProduct(Facing, ScreenRight) > 0.0f;
		ApplyView(EDCSpriteView::Side, bHeadingRight != bSideArtFacesRight);
	}
}
