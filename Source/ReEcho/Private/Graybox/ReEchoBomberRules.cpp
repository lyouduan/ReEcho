#include "Graybox/ReEchoBomberRules.h"

namespace ReEchoBomberRules
{
bool ShouldStartFuse(const float Distance, const float TriggerRadius)
{
	return Distance <= FMath::Max(0.0f, TriggerRadius);
}

bool IsInsideExplosion(const float Distance, const float DamageRadius)
{
	return Distance <= FMath::Max(0.0f, DamageRadius);
}
}
