#pragma once

#include "CoreMinimal.h"

namespace ReEchoBomberRules
{
REECHO_API bool ShouldStartFuse(float Distance, float TriggerRadius);
REECHO_API bool IsInsideExplosion(float Distance, float DamageRadius);
}
