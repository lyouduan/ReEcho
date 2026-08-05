#include "Run/ReEchoCharacterPromotion.h"

namespace
{
int32 CountCards(const TArray<FName>& Cards, const TArray<FName>& Category)
{
	int32 Score = 0;
	for (const FName Card : Cards)
	{
		Score += Category.Contains(Card) ? 1 : 0;
	}
	return Score;
}
}

namespace ReEchoCharacterPromotion
{
FName EvaluateRole(const TArray<FName>& Cards)
{
	const int32 Physical = CountCards(Cards, {TEXT("G_1_04")});
	const int32 Element = CountCards(Cards, {TEXT("G_1_05")});
	const int32 Survival = CountCards(Cards, {TEXT("G_1_02"), TEXT("G_1_03")});
	const int32 Utility = CountCards(Cards, {TEXT("G_1_01"), TEXT("G_1_08")});
	const int32 Best = FMath::Max(FMath::Max(Physical, Element), FMath::Max(Survival, Utility));
	if (Physical == Best)
	{
		return TEXT("Hunter");
	}
	if (Element == Best)
	{
		return TEXT("Poet");
	}
	if (Survival == Best)
	{
		return TEXT("Brave");
	}
	return TEXT("Sage");
}

bool TryPromote(FReEchoBuildSnapshot& Build)
{
	if (Build.Cards.Num() != 4 || Build.RuleFlags.Contains(TEXT("Promoted")))
	{
		return false;
	}

	const FName Role = EvaluateRole(Build.Cards);
	Build.RuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	Build.RuleFlags.Add(TEXT("Role"), Role.ToString());
	Build.Stats.RoleId = Role;
	if (Role == TEXT("Hunter"))
	{
		Build.CharacterId = TEXT("J_DIAMOND");
		Build.Stats.HpMax -= 3.0f;
		Build.Stats.PhysicalAttack += 3.0f;
		Build.Stats.ElementalAttack -= 3.0f;
		Build.Stats.CriticalRate += 0.20f;
		Build.Stats.CriticalEffect += 0.50f;
	}
	else if (Role == TEXT("Poet"))
	{
		Build.CharacterId = TEXT("J_CLOVER");
		Build.Stats.HpMax -= 7.0f;
		Build.Stats.PhysicalAttack -= 3.0f;
		Build.Stats.ElementalAttack += 5.0f;
		Build.Stats.bRandomElementProjectiles = true;
	}
	else if (Role == TEXT("Brave"))
	{
		Build.CharacterId = TEXT("J_HEART");
		Build.Stats.HpMax += 5.0f;
		Build.Stats.PhysicalAttack -= 3.0f;
		Build.Stats.ElementalAttack -= 3.0f;
		Build.Stats.EverySecondAttackBonus = 0.5f;
	}
	else
	{
		Build.CharacterId = TEXT("J_SPADE");
	}

	Build.Stats.HpMax = FMath::Max(1.0f, Build.Stats.HpMax);
	Build.Stats.PhysicalAttack = FMath::Max(0.0f, Build.Stats.PhysicalAttack);
	Build.Stats.ElementalAttack = FMath::Max(0.0f, Build.Stats.ElementalAttack);
	return true;
}

bool IsRole(const FReEchoBuildSnapshot& Build, const FName RoleId)
{
	return Build.Stats.RoleId == RoleId;
}
}
