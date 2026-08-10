#include "Run/ReEchoCharacterPromotion.h"

#include "Data/ReEchoCsvDataRegistry.h"

namespace
{
void ApplyStatDelta(FReEchoStatBlock& Target, const FReEchoStatBlock& From, const FReEchoStatBlock& To)
{
	Target.HpMax += To.HpMax - From.HpMax;
	Target.HpPoint = FMath::Min(Target.HpPoint, Target.HpMax);
	Target.PhysicalAttack += To.PhysicalAttack - From.PhysicalAttack;
	Target.ElementalAttack += To.ElementalAttack - From.ElementalAttack;
	Target.AttackSpeed += To.AttackSpeed - From.AttackSpeed;
	Target.MovementSpeed += To.MovementSpeed - From.MovementSpeed;
	Target.CriticalRate += To.CriticalRate - From.CriticalRate;
	Target.CriticalEffect += To.CriticalEffect - From.CriticalEffect;
	Target.EchoEfficiency += To.EchoEfficiency - From.EchoEfficiency;
	Target.ReactionEfficiency += To.ReactionEfficiency - From.ReactionEfficiency;
	Target.EverySecondAttackBonus = To.EverySecondAttackBonus;
	Target.bRandomElementProjectiles = To.bRandomElementProjectiles;
}

const FReEchoCsvCharacterRow* FindCharacterForRole(const FReEchoCsvDataSnapshot& Snapshot, const FName RoleId)
{
	const FReEchoCsvCharacterRow* Best = nullptr;
	for (const TPair<FName, FReEchoCsvCharacterRow>& Pair : Snapshot.Characters)
	{
		const FReEchoCsvCharacterRow& Character = Pair.Value;
		if (!Character.bEnabled || Character.RoleId != RoleId)
		{
			continue;
		}
		if (!Best || Character.PromotionPriority < Best->PromotionPriority)
		{
			Best = &Character;
		}
	}
	return Best;
}
}

namespace ReEchoCharacterPromotion
{
FName EvaluateRole(const TArray<FName>& Cards)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return NAME_None;
	}

	TMap<FName, int32> Scores;
	for (const TPair<FName, FReEchoCsvCharacterRow>& Pair : Snapshot->Characters)
	{
		const FReEchoCsvCharacterRow& Character = Pair.Value;
		if (Character.bEnabled && Character.RoleId != TEXT("None"))
		{
			Scores.Add(Character.RoleId, 0);
		}
	}
	for (const FName CardId : Cards)
	{
		const FReEchoCsvCardRow* Card = Snapshot->FindCard(CardId);
		if (Card && Card->bEnabled && Scores.Contains(Card->PromotionRoleId))
		{
			++Scores.FindOrAdd(Card->PromotionRoleId);
		}
	}

	const FReEchoCsvCharacterRow* BestCharacter = nullptr;
	int32 BestScore = MIN_int32;
	for (const TPair<FName, int32>& ScorePair : Scores)
	{
		const FReEchoCsvCharacterRow* Character = FindCharacterForRole(*Snapshot, ScorePair.Key);
		if (!Character)
		{
			continue;
		}
		if (ScorePair.Value > BestScore ||
		    (ScorePair.Value == BestScore &&
		     (!BestCharacter || Character->PromotionPriority < BestCharacter->PromotionPriority)))
		{
			BestCharacter = Character;
			BestScore = ScorePair.Value;
		}
	}
	return BestCharacter ? BestCharacter->RoleId : NAME_None;
}

bool TryPromote(FReEchoBuildSnapshot& Build)
{
	if (Build.Cards.Num() != 4 || Build.RuleFlags.Contains(TEXT("Promoted")))
	{
		return false;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return false;
	}

	const FName Role = EvaluateRole(Build.Cards);
	const FReEchoCsvCharacterRow* TargetCharacter = FindCharacterForRole(*Snapshot, Role);
	const FName BaseCharacterId(*Build.RuleFlags.FindRef(TEXT("BaseCharacterId")));
	const FReEchoCsvCharacterRow* BaseCharacter =
	    Snapshot->FindCharacter(BaseCharacterId.IsNone() ? Build.CharacterId : BaseCharacterId);
	if (!TargetCharacter || !BaseCharacter)
	{
		return false;
	}

	Build.RuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	Build.RuleFlags.Add(TEXT("Role"), Role.ToString());
	Build.Stats.RoleId = Role;
	Build.CharacterId = TargetCharacter->Id;
	ApplyStatDelta(Build.Stats, BaseCharacter->BaseStats, TargetCharacter->BaseStats);

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
