// Director's Cut - hand-drawn set dressing (Taisho's props) as camera-facing cut-outs you can drop in a level.

#pragma once

#include "CoreMinimal.h"
#include "DirectorsCut/Rendering/DCSpriteActor.h"
#include "DCSetProps.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;

/**
 *  A hand-drawn set prop. Two looks:
 *  - 3D (default when a Mesh is set): Taisho's drawing rebuilt in Blender as a real model painted with the drawing
 *    (Scripts/Blender_BuildProps3D.py), so every side looks right.
 *  - 2D: the original drawing as a camera-facing card (feet on the ground) - turn off bUse3DModel.
 *  Plus an optional invisible blocker so the player and boars bump into big props. Drop a ready-made child in a level (DC Prop Grass 01, DC Prop Hay Bale 01),
 *  or place this class and set the Sprite's Front Texture / Front Size per instance.
 */
UCLASS(DisplayName="DC Set Prop")
class EPICGAMEJAMGAME_API ADCSetProp : public ADCSpriteActor
{
	GENERATED_BODY()

public:

	ADCSetProp();

	/** Show the 3D model (Mesh) instead of the camera-facing card; off = the original 2D cut-out */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop")
	bool bUse3DModel = true;

	/** Invisible capsule that blocks pawns (off for small props like grass) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop")
	bool bBlocksPawns = false;

	/** Random scale range per placed copy (1-1 = all the same); keeps a field of copies from looking stamped */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prop")
	FVector2D RandomScale = FVector2D(1.0f, 1.0f);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> Blocker;

	/** The 3D version of the drawing (SM_DC_*_3D) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Shows the mesh or the card depending on bUse3DModel (and whether a mesh exists) */
	void ApplyLook();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
};

/** Taisho's grass tuft (grass_01): ~1.4 m wide, no collision, slight random size */
UCLASS(DisplayName="DC Prop Grass 01")
class EPICGAMEJAMGAME_API ADCProp_Grass01 : public ADCSetProp
{
	GENERATED_BODY()
public:
	ADCProp_Grass01();
};

/** Taisho's hay bales with pitchfork (hay_bale_01): ~3 m wide, blocks the player and boars */
UCLASS(DisplayName="DC Prop Hay Bale 01")
class EPICGAMEJAMGAME_API ADCProp_HayBale01 : public ADCSetProp
{
	GENERATED_BODY()
public:
	ADCProp_HayBale01();
};

/** Taisho's apple tree (Apple_Tree_02): ~5.5 m tall; only the trunk blocks, so you can walk under the canopy */
UCLASS(DisplayName="DC Prop Apple Tree 02")
class EPICGAMEJAMGAME_API ADCProp_AppleTree02 : public ADCSetProp
{
	GENERATED_BODY()
public:
	ADCProp_AppleTree02();
};
