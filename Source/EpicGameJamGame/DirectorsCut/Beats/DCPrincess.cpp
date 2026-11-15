// Director's Cut - Beat 4 Princess actor guarded by the King Boar.

#include "DirectorsCut/Beats/DCPrincess.h"
#include "Quest/DCQuestSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/LightComponent.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
#include "DirectorsCut/Rendering/DCBillboardSprite.h"
#include "Engine/Texture2D.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ADCPrincess::ADCPrincess()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	RescueTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("Rescue Trigger"));
	RescueTrigger->SetupAttachment(Root);
	RescueTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	RescueTrigger->InitSphereRadius(165.0f);
	RescueTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RescueTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RescueTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// the janitor princess: a camera-facing card (1024 x 1536 drawing, about 1.9 m tall, feet on the ground)
	PrincessSprite = CreateDefaultSubobject<UDCBillboardSprite>(TEXT("Princess Sprite"));
	PrincessSprite->SetupAttachment(Root);
	PrincessSprite->SetRelativeLocation(FVector(0.0f, 0.0f, -4.0f));   // boots sit ~2% above the drawing's bottom edge
	PrincessSprite->FrontSize = FVector2D(127.0f, 190.0f);
	PrincessSprite->bPivotAtBottom = true;
	static ConstructorHelpers::FObjectFinder<UTexture2D> PrincessArt(TEXT("/Game/DirectorsCut/Characters/Princess/T_DC_Princess_Janitor.T_DC_Princess_Janitor"));
	if (PrincessArt.Succeeded())
	{
		PrincessSprite->FrontTexture = PrincessArt.Object;
	}

	RomanceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Romance Light"));
	RomanceLight->SetupAttachment(Root);
	RomanceLight->SetRelativeLocation(FVector(0.0f, 0.0f, 240.0f));
	RomanceLight->SetLightColor(FLinearColor(1.0f, 0.45f, 0.8f));
	RomanceLight->SetIntensity(4500.0f);
	RomanceLight->SetAttenuationRadius(750.0f);

	PrincessLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Princess Label"));
	PrincessLabel->SetupAttachment(Root);
	PrincessLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 215.0f));
	PrincessLabel->SetHorizontalAlignment(EHTA_Center);
	PrincessLabel->SetWorldSize(36.0f);
	PrincessLabel->SetText(FText::FromString(TEXT("PRINCESS (GUARDED)")));
	PrincessLabel->SetTextRenderColor(FColor(255, 130, 210));
	PrincessLabel->SetUsingAbsoluteRotation(true);

	Tags.Add(TEXT("Princess"));
}

void ADCPrincess::BeginPlay()
{
	Super::BeginPlay();
	RescueTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADCPrincess::HandleOverlap);
}

void ADCPrincess::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BobTime += DeltaSeconds;
	UpdateShedLights(DeltaSeconds);

	if (PrincessLabel)
	{
		if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
		{
			PrincessLabel->SetWorldRotation(FRotator(0.0f, (Player->GetActorLocation() - PrincessLabel->GetComponentLocation()).Rotation().Yaw, 0.0f));
		}
	}
}

void ADCPrincess::UnlockPrincess()
{
	bLocked = false;
	if (PrincessLabel)
	{
		PrincessLabel->SetText(FText::FromString(TEXT("RESCUE THE PRINCESS!")));
		PrincessLabel->SetTextRenderColor(FColor(120, 255, 160));
	}

	// If player is already standing inside the rescue trigger when unlocking, complete immediately
	TArray<AActor*> Overlapping;
	RescueTrigger->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* A : Overlapping)
	{
		if (APawn* P = Cast<APawn>(A); P && P->IsPlayerControlled())
		{
			HandleOverlap(RescueTrigger, P, nullptr, 0, false, FHitResult());
			break;
		}
	}
}

void ADCPrincess::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bRescued)
	{
		return;
	}
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (bLocked)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastLockedPromptTime >= 6.0f)
		{
			LastLockedPromptTime = Now;
			OnLockedAttempt.Broadcast();
		}
		return;
	}

	bRescued = true;
	if (PrincessLabel)
	{
		PrincessLabel->SetText(FText::FromString(TEXT("PRINCESS RESCUED!")));
	}
	if (UDCQuestSubsystem* Quest = GetWorld()->GetSubsystem<UDCQuestSubsystem>())
	{
		Quest->ReportEvent(FName("Rescue.Princess"));
	}
	OnPrincessRescued.Broadcast();
}

void ADCPrincess::GatherShedLights()
{
	bShedLightsGathered = true;
	ShedLights.Reset();
	ShedLightBaseIntensity.Reset();

	auto Add = [this](ULightComponent* L)
	{
		if (L && L->Mobility != EComponentMobility::Static && !ShedLights.Contains(L))
		{
			ShedLights.Add(L);
			ShedLightBaseIntensity.Add(L->Intensity);
		}
	};
	Add(RomanceLight);

	// the shed's own lamp (placed in DC_Set_PG); the shed doors' lock glow and anything further away are left alone
	const FVector Here = GetActorLocation();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (A == this || A->GetClass()->GetName().Contains(TEXT("ShedDoors")))
		{
			continue;
		}
		TArray<ULightComponent*> Lights;
		A->GetComponents<ULightComponent>(Lights);
		for (ULightComponent* L : Lights)
		{
			if (FVector::Dist2D(L->GetComponentLocation(), Here) <= ShedLightSearchRadius && FMath::Abs(L->GetComponentLocation().Z - Here.Z) < 800.0f)
			{
				Add(L);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Princess: %d light(s) fade when the player is inside the shed"), ShedLights.Num());
}

void ADCPrincess::UpdateShedLights(float DeltaSeconds)
{
	if (!bShedLightsGathered)
	{
		GatherShedLights();          // first tick: the set's lights are loaded by then
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bInside = Player && FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) <= InsideRadius;
	const float Target = bInside ? InsideLightScale : 1.0f;
	if (FMath::IsNearlyEqual(LightAlpha, Target, 0.001f))
	{
		return;
	}
	LightAlpha = FMath::FInterpConstantTo(LightAlpha, Target, DeltaSeconds, (1.0f - InsideLightScale) / FMath::Max(LightFadeTime, 0.05f));

	// ease in and out, so the reveal starts and ends softly
	const float Eased = FMath::SmoothStep(InsideLightScale, 1.0f, LightAlpha) * (1.0f - InsideLightScale) + InsideLightScale;
	for (int32 i = 0; i < ShedLights.Num(); ++i)
	{
		if (ULightComponent* L = ShedLights[i].Get())
		{
			L->SetIntensity(ShedLightBaseIntensity[i] * Eased);
		}
	}
}
