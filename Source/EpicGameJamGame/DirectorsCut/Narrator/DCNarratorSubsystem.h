// Director's Cut - the Director's voice (subtitles now, voiceover later).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCNarratorSubsystem.generated.h"

class UAudioComponent;
class USoundBase;

/** One Director line. Row type of DT_NarratorLines. */
USTRUCT(BlueprintType)
struct FDCNarratorLine : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Narrator")
	FText Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Narrator", meta=(MultiLine=true))
	FText Text;

	/** Seconds on screen. Ignored when Voice is set and longer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Narrator", meta=(ClampMin=0))
	float Duration = 3.0f;

	/** Optional voiceover clip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Narrator")
	TObjectPtr<USoundBase> Voice;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDCOnLineStarted, FText, Speaker, FText, Text, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDCOnLineFinished, FName, RowId);

/**
 *  Plays Director lines from DT_NarratorLines, one at a time, with a queue.
 *  The subtitle widget listens to OnLineStarted / OnLineFinished.
 *  The table is set in DefaultGame.ini: [/Script/EpicGameJamGame.DCNarratorSubsystem] LinesTable=...
 *  Console: DC.PlayLine <RowId> [RowId...], DC.InterruptLine
 */
UCLASS(Config=Game)
class EPICGAMEJAMGAME_API UDCNarratorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Queues a line by row id. Plays immediately if nothing is playing. */
	UFUNCTION(BlueprintCallable, Category="Narrator")
	void PlayLine(FName RowId);

	/** Queues several lines to play back to back */
	UFUNCTION(BlueprintCallable, Category="Narrator")
	void PlaySequence(const TArray<FName>& RowIds);

	/** Plays one line chosen at random from RowIds */
	UFUNCTION(BlueprintCallable, Category="Narrator")
	void PlayRandomLine(const TArray<FName>& RowIds);

	/** Alternates between PrimaryRowId and AltRowId each time this pair is triggered */
	UFUNCTION(BlueprintCallable, Category="Narrator")
	void PlayAlternatingLine(FName PrimaryRowId, FName AltRowId);

	/** Cuts off the current line and clears the queue. Fires OnLineFinished for the cut-off line. */
	UFUNCTION(BlueprintCallable, Category="Narrator")
	void Interrupt();

	UFUNCTION(BlueprintPure, Category="Narrator")
	bool IsPlaying() const { return !CurrentLineId.IsNone(); }

	UFUNCTION(BlueprintPure, Category="Narrator")
	FName GetCurrentLineId() const { return CurrentLineId; }

	UPROPERTY(BlueprintAssignable, Category="Narrator")
	FDCOnLineStarted OnLineStarted;

	UPROPERTY(BlueprintAssignable, Category="Narrator")
	FDCOnLineFinished OnLineFinished;

	/** Playback volume multiplier for Director voiceover clips */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Narrator", meta=(ClampMin=0.1, ClampMax=4.0))
	float VoiceVolume = 1.6f;

	void FindRowNamesForDebug();

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

private:

	const FDCNarratorLine* FindLine(FName RowId);
	void StartNextLine();
	void FinishCurrentLine();
	void StopVoice();

	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> LinesTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> LoadedTable;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> VoiceComponent;

	TArray<FName> Queue;
	FName CurrentLineId;
	FTimerHandle LineTimer;
	TMap<FName, bool> AlternateToggle;
};
