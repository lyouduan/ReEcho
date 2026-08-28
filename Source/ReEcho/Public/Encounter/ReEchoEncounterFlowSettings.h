#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReEchoEncounterFlowSettings.generated.h"

/**
 * Encounter-flow tuning owned by the dedicated BP_EncounterFlowSettings Blueprint.
 *
 * Keep cross-encounter protection and transition tuning here instead of scattering
 * designer-facing constants through GameMode.
 */
UCLASS(BlueprintType, Blueprintable)
class REECHO_API UReEchoEncounterFlowSettings : public UObject
{
	GENERATED_BODY()

public:
	/** Invulnerability granted whenever the player regains control at the start of an Encounter. Zero disables it. */
	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Encounter|Player Protection",
	          meta = (ClampMin = "0.0", UIMin = "0.0", Units = "s"))
	float PostEntryInvulnerabilitySeconds = 0.5f;
};
