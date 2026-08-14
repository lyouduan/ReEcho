#pragma once

#include "Combat/ReEchoCombatTypes.h"

/** Pure element predicates with no world, data-registry, or concrete actor dependency. */
namespace ReEchoCombatElementRules
{
REECHOCOMBAT_API bool IsCombatElement(EReEchoElement Element);
REECHOCOMBAT_API bool IsDoubleDamagePair(EReEchoElement First, EReEchoElement Second);
}
