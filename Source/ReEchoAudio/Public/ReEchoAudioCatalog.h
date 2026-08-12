#pragma once

#include "CoreMinimal.h"
#include "ReEchoAudioTypes.h"

/**
 * Catalog provider seam.
 *
 * The policy engine only ever talks to this interface, never to a concrete
 * table. Plan33 ships an empty built-in catalog; Plan34 plugs in a provider
 * that loads definitions from designer tables without touching gameplay code.
 */
class REECHOAUDIO_API IReEchoAudioCatalogProvider
{
public:
	virtual ~IReEchoAudioCatalogProvider() = default;

	/** Returns the definition for an event/state id, or nullptr if unknown. */
	virtual const FReEchoAudioEventDefinition* FindDefinition(FName EventId) const = 0;
};

/**
 * In-memory catalog. Default provider for Plan33; also the test/Plan34
 * injection point. Gameplay callers must NOT register into it directly.
 */
class REECHOAUDIO_API FReEchoAudioCatalog : public IReEchoAudioCatalogProvider
{
public:
	/** Add or replace a definition (test/provider seam only). */
	void AddDefinition(const FReEchoAudioEventDefinition& Definition);

	/** Clear all definitions (test seam only). */
	void Clear();

	virtual const FReEchoAudioEventDefinition* FindDefinition(FName EventId) const override;

private:
	TMap<FName, FReEchoAudioEventDefinition> Definitions;
};
