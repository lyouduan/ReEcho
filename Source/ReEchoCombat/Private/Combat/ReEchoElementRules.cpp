#include "Combat/ReEchoElementRules.h"

bool ReEchoCombatElementRules::IsCombatElement(const EReEchoElement Element)
{
	return Element == EReEchoElement::Flame || Element == EReEchoElement::Lightning ||
	       Element == EReEchoElement::Grass || Element == EReEchoElement::Water;
}

bool ReEchoCombatElementRules::IsDoubleDamagePair(const EReEchoElement First, const EReEchoElement Second)
{
	return (First == EReEchoElement::Flame && Second == EReEchoElement::Water) ||
	       (First == EReEchoElement::Water && Second == EReEchoElement::Flame);
}
