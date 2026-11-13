// Director's Cut - quest tracking.

#include "Quest/DCQuestSubsystem.h"

bool UDCQuestSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDCQuestSubsystem::StartQuest(FName Id, FText Title, int32 Target, FName EventTag)
{
	// TODO(Lead): stub
}

void UDCQuestSubsystem::ReportEvent(FName EventTag)
{
	// TODO(Lead): stub
}

void UDCQuestSubsystem::ReplaceQuest(FName Id, FText Title, int32 Target, FName EventTag, bool bKeepProgress)
{
	// TODO(Lead): stub
}
