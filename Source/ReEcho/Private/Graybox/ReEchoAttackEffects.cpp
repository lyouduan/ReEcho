#include "Graybox/ReEchoAttackEffects.h"

#include "Graybox/ReEchoHitImpactActor.h"

void ReEchoAttackEffects::SpawnHitImpact(UWorld* World, const FVector& Location)
{
	if (!World)
	{
		return;
	}
	World->SpawnActor<AReEchoHitImpactActor>(
		Location + FVector(0.0f, 0.0f, 45.0f),
		FRotator::ZeroRotator);
}