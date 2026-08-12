#include "ReEchoAudioCatalog.h"

void FReEchoAudioCatalog::AddDefinition(const FReEchoAudioEventDefinition& Definition)
{
	Definitions.Add(Definition.EventId, Definition);
}

void FReEchoAudioCatalog::Clear()
{
	Definitions.Empty();
}

const FReEchoAudioEventDefinition* FReEchoAudioCatalog::FindDefinition(FName EventId) const
{
	return Definitions.Find(EventId);
}
