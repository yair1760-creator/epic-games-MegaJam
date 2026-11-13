// Director's Cut - the Director's voice (subtitles now, voiceover later).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCNarratorSubsystem.generated.h"

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
 */
UCLASS()
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

	/** Cuts off the current line and clears the queue */
	UFUNCTION(BlueprintCallable, Category="Narrator")
	void Interrupt();

	UPROPERTY(BlueprintAssignable, Category="Narrator")
	FDCOnLineStarted OnLineStarted;

	UPROPERTY(BlueprintAssignable, Category="Narrator")
	FDCOnLineFinished OnLineFinished;

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
