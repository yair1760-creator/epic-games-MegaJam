// Rated PG for Pig - loading screen.

#include "UI/DCLoadingScreen.h"
#include "EpicGameJamGame.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformTime.h"
#include "Misc/CoreDelegates.h"
#include "MoviePlayer.h"
#include "Slate/DeferredCleanupSlateBrush.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include <atomic>

/** Shared between the game thread (the subsystem) and the loading thread (the widget). */
struct FDCLoadingAnimState
{
	TArray<TSharedPtr<FDeferredCleanupSlateBrush>> BoarBrushes;
	TSharedPtr<FDeferredCleanupSlateBrush> TextBrush;
	double StartSeconds = 0.0;
	float AnimFPS = 12.0f;

	// what was really drawn, for the log line at the end of the load
	std::atomic<int32> LastFrame{ -1 };
	std::atomic<int32> FrameSwitches{ 0 };
	std::atomic<int32> FramesSeenMask{ 0 };
	std::atomic<int32> BrushQueries{ 0 };

	void Reset(float InFPS)
	{
		StartSeconds = FPlatformTime::Seconds();
		AnimFPS = FMath::Max(InFPS, 1.0f);
		LastFrame = -1;
		FrameSwitches = 0;
		FramesSeenMask = 0;
		BrushQueries = 0;
	}

	/** Frame to show right now: 01..08 in ascending order, each held 1 / AnimFPS s, then straight back to 01 */
	int32 CurrentFrame() const
	{
		const int32 Num = BoarBrushes.Num();
		if (Num <= 0)
		{
			return 0;
		}
		const double Elapsed = FMath::Max(FPlatformTime::Seconds() - StartSeconds, 0.0);
		return int32(FMath::FloorToDouble(Elapsed * double(AnimFPS))) % Num;
	}
};

class SDCLoadingScreen : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SDCLoadingScreen) {}
		SLATE_ARGUMENT(TSharedPtr<FDCLoadingAnimState>, State)
		SLATE_ARGUMENT(FLinearColor, BackdropColor)
		SLATE_ARGUMENT(float, IconScale)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		State = InArgs._State;
		const float Scale = FMath::Max(InArgs._IconScale, 0.1f);

		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(InArgs._BackdropColor)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(0.0f)
			[
				SNew(SDPIScaler)
				.DPIScale(this, &SDCLoadingScreen::GetDPIScale)
				[
					// one box for both layers: identical size, anchor and position, transparent padding kept
					SNew(SBox)
					.WidthOverride(256.0f * Scale)
					.HeightOverride(212.0f * Scale)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SImage).Image(this, &SDCLoadingScreen::GetBoarBrush)      // the only layer that changes
						]
						+ SOverlay::Slot()
						[
							SNew(SImage).Image(State.IsValid() && State->TextBrush.IsValid() ? State->TextBrush->GetSlateBrush() : nullptr)
						]
					]
				]
			]
		];
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		const float Height = AllottedGeometry.GetLocalSize().Y;
		if (Height > 1.0f)
		{
			ViewHeight = Height;
		}
	}

private:

	/** Same rule as the UMG menus: sizes are given for a 1080-high screen */
	float GetDPIScale() const
	{
		return FMath::Clamp(ViewHeight / 1080.0f, 0.25f, 4.0f);
	}

	const FSlateBrush* GetBoarBrush() const
	{
		if (!State.IsValid() || State->BoarBrushes.Num() == 0)
		{
			return nullptr;
		}
		const int32 Frame = State->CurrentFrame();
		++State->BrushQueries;
		if (State->LastFrame.exchange(Frame) != Frame)
		{
			++State->FrameSwitches;
			State->FramesSeenMask |= (1 << Frame);
		}
		const TSharedPtr<FDeferredCleanupSlateBrush>& Brush = State->BoarBrushes[Frame];
		return Brush.IsValid() ? Brush->GetSlateBrush() : nullptr;
	}

	TSharedPtr<FDCLoadingAnimState> State;
	float ViewHeight = 1080.0f;
};

void UDCLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IsRunningDedicatedServer())
	{
		return;
	}

	for (int32 i = 1; i <= 8; ++i)
	{
		const FString Path = FString::Printf(TEXT("/Game/DirectorsCut/UI/Loading/T_UI_Loading_Boar_%02d.T_UI_Loading_Boar_%02d"), i, i);
		if (UTexture2D* Frame = LoadObject<UTexture2D>(nullptr, *Path))
		{
			BoarFrames.Add(Frame);
		}
	}
	TextTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/DirectorsCut/UI/Loading/T_UI_Loading_Text.T_UI_Loading_Text"));

	AnimState = MakeShared<FDCLoadingAnimState>();
	const FVector2D Canvas(256.0, 212.0);
	for (UTexture2D* Frame : BoarFrames)
	{
		AnimState->BoarBrushes.Add(FDeferredCleanupSlateBrush::CreateBrush(Frame, Canvas));
	}
	if (TextTexture)
	{
		AnimState->TextBrush = FDeferredCleanupSlateBrush::CreateBrush(TextTexture, Canvas);
	}

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UDCLoadingScreenSubsystem::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UDCLoadingScreenSubsystem::HandlePostLoadMap);
}

void UDCLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	AnimState.Reset();
	Super::Deinitialize();
}

void UDCLoadingScreenSubsystem::HandlePreLoadMap(const FString& MapName)
{
	LoadingMapName = MapName;
	if (!AnimState.IsValid() || AnimState->BoarBrushes.Num() == 0 || !GetMoviePlayer())
	{
		return;
	}

	// loading begins: both layers go up together, the boar starts on drawing 01
	AnimState->Reset(AnimFPS);

	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;      // hidden as soon as the game is ready...
	Attributes.MinimumLoadingScreenDisplayTime = MinimumDisplaySeconds;   // ...but never before this
	Attributes.bMoviesAreSkippable = false;
	Attributes.bWaitForManualStop = false;
	Attributes.bAllowEngineTick = false;
	Attributes.WidgetLoadingScreen = SNew(SDCLoadingScreen)
		.State(AnimState)
		.BackdropColor(BackdropColor)
		.IconScale(IconScale);
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

void UDCLoadingScreenSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!AnimState.IsValid())
	{
		return;
	}
	LastShownSeconds = float(FPlatformTime::Seconds() - AnimState->StartSeconds);
	LastFrameSwitches = AnimState->FrameSwitches.load();
	const int32 Mask = AnimState->FramesSeenMask.load();
	LastDistinctFrames = FMath::CountBits(uint32(Mask));
	UE_LOG(LogEpicGameJamGame, Log, TEXT("DCLoadingScreen: '%s' loaded after %.2f s; boar drawn %d times, %d frame switches, %d of %d drawings shown (mask 0x%02X)"),
		*LoadingMapName, LastShownSeconds, AnimState->BrushQueries.load(), LastFrameSwitches, LastDistinctFrames, AnimState->BoarBrushes.Num(), Mask);
}
