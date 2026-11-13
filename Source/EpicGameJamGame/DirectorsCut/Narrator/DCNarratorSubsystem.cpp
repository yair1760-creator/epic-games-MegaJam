// Director's Cut - the Director's voice.

#include "Narrator/DCNarratorSubsystem.h"

bool UDCNarratorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDCNarratorSubsystem::PlayLine(FName RowId)
{
	// TODO(Lead): stub
}

void UDCNarratorSubsystem::PlaySequence(const TArray<FName>& RowIds)
{
	// TODO(Lead): stub
}

void UDCNarratorSubsystem::Interrupt()
{
	// TODO(Lead): stub
}
