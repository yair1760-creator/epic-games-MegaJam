// Director's Cut - HUD player health bar.

#include "UI/DCHealthBarWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "ShooterCharacter.h"

UDCHealthBarWidget::UDCHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Empty(TEXT("/Game/DirectorsCut/UI/HUD/T_UI_HPBar_Empty.T_UI_HPBar_Empty"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> Full(TEXT("/Game/DirectorsCut/UI/HUD/T_UI_HPBar_Full.T_UI_HPBar_Full"));
	if (Empty.Succeeded()) BarEmptyTexture = Empty.Object;
	if (Full.Succeeded())  BarFullTexture = Full.Object;
}

void UDCHealthBarWidget::BuildDrawnBar()
{
	if (!bUseDrawnBar || FillImage || !BarEmptyTexture || !BarFullTexture || !WidgetTree)
	{
		return;
	}
	// the pictures go into the same stack the plain bar sits in (WBP_HealthBar: SizeBox > Overlay > bar + number)
	UOverlay* Stack = HealthBar ? Cast<UOverlay>(HealthBar->GetParent()) : Cast<UOverlay>(WidgetTree->RootWidget);
	if (!Stack)
	{
		return;       // restyled layout without that stack: leave it as its designer made it
	}

	auto AddLayer = [this, Stack](UTexture2D* Texture, FName Name) -> UImage*
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = BarSize;
		Image->SetBrush(Brush);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* LayerSlot = Stack->AddChildToOverlay(Image))
		{
			LayerSlot->SetHorizontalAlignment(HAlign_Left);      // both layers share the same top-left corner
			LayerSlot->SetVerticalAlignment(VAlign_Top);
		}
		return Image;
	};
	EmptyImage = AddLayer(BarEmptyTexture, TEXT("HPBarEmpty"));
	FillImage = AddLayer(BarFullTexture, TEXT("HPBarFull"));

	if (HealthBar)
	{
		HealthBar->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (HealthText && !bShowNumber)
	{
		HealthText->SetVisibility(ESlateVisibility::Collapsed);
	}
	// the layout's frame was sized for the plain bar: make room for the drawn one
	for (UWidget* W = Stack->GetParent(); W; W = W->GetParent())
	{
		if (USizeBox* Box = Cast<USizeBox>(W))
		{
			Box->SetWidthOverride(BarSize.X);
			Box->SetHeightOverride(BarSize.Y);
			break;
		}
	}
}

void UDCHealthBarWidget::UpdateDrawnFill()
{
	if (!FillImage || !BarFullTexture)
	{
		return;
	}
	// clip edge on the inner track: 0 % = left edge of the track, 100 % = its right edge. The full picture is
	// never resized: the image shows only the left part of the texture, at the same scale as the empty one.
	const float ArtWidth = FMath::Max(float(BarFullTexture->GetSizeX()), 1.0f);
	const float EdgePx = TrackStartPx + FMath::Clamp(LifePercent, 0.0f, 1.0f) * (TrackEndPx - TrackStartPx);
	FillClipFraction = FMath::Clamp(EdgePx / ArtWidth, 0.0f, 1.0f);

	FSlateBrush Brush;
	Brush.SetResourceObject(BarFullTexture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = FVector2D(BarSize.X * FillClipFraction, BarSize.Y);
	Brush.SetUVRegion(FBox2f(FVector2f(0.0f, 0.0f), FVector2f(FillClipFraction, 1.0f)));
	FillImage->SetBrush(Brush);
}

void UDCHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildDrawnBar();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UDCHealthBarWidget::HandlePawnChanged);
		HandlePawnChanged(nullptr, PC->GetPawn());
	}
	else
	{
		Apply(1.0f);
	}
}

void UDCHealthBarWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveAll(this);
	}
	Unbind();
	Super::NativeDestruct();
}

void UDCHealthBarWidget::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	Unbind();
	if (AShooterCharacter* Character = Cast<AShooterCharacter>(NewPawn))
	{
		Character->OnDamaged.AddUniqueDynamic(this, &UDCHealthBarWidget::HandleDamaged);
		BoundCharacter = Character;
		Apply(Character->GetHPPercent());
	}
}

void UDCHealthBarWidget::Unbind()
{
	if (AShooterCharacter* Character = BoundCharacter.Get())
	{
		Character->OnDamaged.RemoveAll(this);
	}
	BoundCharacter.Reset();
}

void UDCHealthBarWidget::HandleDamaged(float NewPercent)
{
	Apply(NewPercent);
}

void UDCHealthBarWidget::Apply(float NewPercent)
{
	NewPercent = FMath::Clamp(NewPercent, 0.0f, 1.0f);
	const bool bDecreased = NewPercent < LifePercent;
	LifePercent = NewPercent;

	if (HealthBar)
	{
		HealthBar->SetPercent(LifePercent);
	}
	UpdateDrawnFill();
	if (HealthText)
	{
		HealthText->SetText(FText::AsNumber(FMath::RoundToInt(LifePercent * 100.0f)));
	}
	OnHealthChanged(LifePercent, bDecreased);
}
