#include "Weapons/ReEchoWeaponLogic.h"

#include "Combat/ReEchoCombatTarget.h"

#include "GameFramework/Actor.h"

namespace
{
EReEchoElement ResolveDamageChannel(const FName DamageChannelId, const int32 AttackSequence)
{
	if (DamageChannelId == TEXT("Flame"))
	{
		return EReEchoElement::Flame;
	}
	if (DamageChannelId == TEXT("Lightning"))
	{
		return EReEchoElement::Lightning;
	}
	if (DamageChannelId == TEXT("Grass"))
	{
		return EReEchoElement::Grass;
	}
	if (DamageChannelId == TEXT("Water"))
	{
		return EReEchoElement::Water;
	}
	if (DamageChannelId == TEXT("RandomElement"))
	{
		switch ((AttackSequence * 17 + 5) % 4)
		{
			case 0:
				return EReEchoElement::Water;
			case 1:
				return EReEchoElement::Flame;
			case 2:
				return EReEchoElement::Lightning;
			default:
				return EReEchoElement::Grass;
		}
	}
	return EReEchoElement::None;
}

EReEchoElement ResolveCyclingElement(const int32 ElementIndex)
{
	static constexpr EReEchoElement Sequence[] = {
	    EReEchoElement::Water,
	    EReEchoElement::Flame,
	    EReEchoElement::Grass,
	    EReEchoElement::Flame,
	    EReEchoElement::Water,
	    EReEchoElement::Lightning,
	    EReEchoElement::Grass,
	    EReEchoElement::Lightning,
	    EReEchoElement::Water,
	    EReEchoElement::Grass,
	    EReEchoElement::Grass,
	    EReEchoElement::Water,
	};
	return Sequence[ElementIndex % UE_ARRAY_COUNT(Sequence)];
}
} // namespace

bool FReEchoWeaponLogic::Initialize(const FReEchoWeaponDefinition& InDefinition)
{
	if (InDefinition.WeaponId.IsNone() || InDefinition.AttackSteps.IsEmpty() ||
	    InDefinition.AttackIntervalSeconds <= 0.0f)
	{
		return false;
	}
	Definition = InDefinition;
	bInitialized = true;
	Reset();
	return true;
}

void FReEchoWeaponLogic::Tick(const float DeltaSeconds)
{
	const float SafeDelta = FMath::Max(0.0f, DeltaSeconds);
	ReadinessRemainingSeconds = FMath::Max(0.0f, ReadinessRemainingSeconds - SafeDelta);
	BehaviorRemainingSeconds = FMath::Max(0.0f, BehaviorRemainingSeconds - SafeDelta);
	InvulnerableRemainingSeconds = FMath::Max(0.0f, InvulnerableRemainingSeconds - SafeDelta);
}

void FReEchoWeaponLogic::Reset()
{
	ReadinessRemainingSeconds = 0.0f;
	BehaviorRemainingSeconds = 0.0f;
	InvulnerableRemainingSeconds = 0.0f;
	CriticalAccumulator = 0.0f;
	NextStepCursor = 0;
	SuccessfulAttackCount = 0;
	NextElementIndex = 0;
	bHasUnconfirmedCommit = false;
}

bool FReEchoWeaponLogic::TryCommitBasicAttack(AActor* Source,
                                              const FReEchoStatBlock& Stats,
                                              FReEchoWeaponAttackCommit& OutCommit)
{
	return TryCommit(Source, Stats, true, OutCommit);
}

bool FReEchoWeaponLogic::TryCommitActiveAttack(AActor* Source,
                                               const FReEchoStatBlock& Stats,
                                               FReEchoWeaponAttackCommit& OutCommit)
{
	return TryCommit(Source, Stats, false, OutCommit);
}

void FReEchoWeaponLogic::ConfirmLastCommit()
{
	bHasUnconfirmedCommit = false;
}

void FReEchoWeaponLogic::RollbackLastCommit()
{
	if (!bHasUnconfirmedCommit)
	{
		return;
	}
	ReadinessRemainingSeconds = CommitCheckpoint.ReadinessRemainingSeconds;
	BehaviorRemainingSeconds = CommitCheckpoint.BehaviorRemainingSeconds;
	InvulnerableRemainingSeconds = CommitCheckpoint.InvulnerableRemainingSeconds;
	CriticalAccumulator = CommitCheckpoint.CriticalAccumulator;
	NextStepCursor = CommitCheckpoint.NextStepCursor;
	SuccessfulAttackCount = CommitCheckpoint.SuccessfulAttackCount;
	NextElementIndex = CommitCheckpoint.NextElementIndex;
	bHasUnconfirmedCommit = false;
}

FReEchoWeaponSnapshot FReEchoWeaponLogic::GetSnapshot() const
{
	FReEchoWeaponSnapshot Snapshot;
	Snapshot.WeaponId = Definition.WeaponId;
	Snapshot.AttackPatternId = Definition.AttackPatternId;
	if (const FReEchoWeaponStepDefinition* Step = GetNextStep())
	{
		Snapshot.NextAttackStepId = Step->StepId;
	}
	Snapshot.ReadinessRemainingSeconds = ReadinessRemainingSeconds;
	Snapshot.BehaviorRemainingSeconds = BehaviorRemainingSeconds;
	Snapshot.bInvulnerable = InvulnerableRemainingSeconds > 0.0f;
	Snapshot.SuccessfulAttackCount = SuccessfulAttackCount;
	Snapshot.NextElement = PeekNextElement();
	return Snapshot;
}

const FReEchoWeaponDefinition& FReEchoWeaponLogic::GetDefinition() const
{
	return Definition;
}

const FReEchoWeaponStepDefinition* FReEchoWeaponLogic::GetNextStep() const
{
	return Definition.AttackSteps.IsValidIndex(NextStepCursor) ? &Definition.AttackSteps[NextStepCursor] : nullptr;
}

float FReEchoWeaponLogic::GetAttackInterval(const FReEchoStatBlock& Stats) const
{
	return Definition.AttackIntervalSeconds / FMath::Max(0.1f, Stats.AttackSpeed);
}

float FReEchoWeaponLogic::GetCurrentRangeCm() const
{
	const FReEchoWeaponStepDefinition* Step = GetNextStep();
	return Step && Step->RangeCm > 0.0f ? Step->RangeCm : Definition.RangeCm;
}

float FReEchoWeaponLogic::GetOnKillHealPercent() const
{
	return Definition.OnKillHealPercent;
}

EReEchoElement FReEchoWeaponLogic::PeekNextElement() const
{
	return Definition.bUsesCyclingElement ? ResolveCyclingElement(NextElementIndex)
	                                      : ResolveDamageChannel(Definition.DamageChannelId, SuccessfulAttackCount + 1);
}

bool FReEchoWeaponLogic::TryCommit(AActor* Source,
                                   const FReEchoStatBlock& Stats,
                                   const bool bRequireReadiness,
                                   FReEchoWeaponAttackCommit& OutCommit)
{
	OutCommit = {};
	const FReEchoWeaponStepDefinition* Step = GetNextStep();
	if (!bInitialized || !Source || !Step || (bRequireReadiness && ReadinessRemainingSeconds > 0.0f))
	{
		return false;
	}
	CommitCheckpoint.ReadinessRemainingSeconds = ReadinessRemainingSeconds;
	CommitCheckpoint.BehaviorRemainingSeconds = BehaviorRemainingSeconds;
	CommitCheckpoint.InvulnerableRemainingSeconds = InvulnerableRemainingSeconds;
	CommitCheckpoint.CriticalAccumulator = CriticalAccumulator;
	CommitCheckpoint.NextStepCursor = NextStepCursor;
	CommitCheckpoint.SuccessfulAttackCount = SuccessfulAttackCount;
	CommitCheckpoint.NextElementIndex = NextElementIndex;
	bHasUnconfirmedCommit = true;

	FReEchoAttackIdentity Attack;
	Attack.Source = Source;
	Attack.Sequence = ++LastAttackSequence;
	Attack.SourceFaction = ReEchoCombatRelations::ResolveActorFaction(Source);
	Attack.WeaponId = Definition.WeaponId;
	if (bRequireReadiness)
	{
		ReadinessRemainingSeconds = FMath::Max(0.01f, GetAttackInterval(Stats));
	}
	const float ScaledDuration = Step->DurationSeconds / FMath::Max(0.1f, Stats.AttackSpeed);
	BehaviorRemainingSeconds = FMath::Max(0.0f, ScaledDuration);
	if (Step->bInvulnerable)
	{
		InvulnerableRemainingSeconds = FMath::Max(InvulnerableRemainingSeconds, ScaledDuration);
	}

	const EReEchoElement Element = ResolveElement();
	bool bCritical = false;
	OutCommit.Attack = Attack;
	OutCommit.WeaponId = Definition.WeaponId;
	OutCommit.AttackPatternId = Definition.AttackPatternId;
	OutCommit.AttackStepId = Step->StepId;
	OutCommit.StepIndex = Step->StepIndex;
	OutCommit.Carrier = Step->Carrier;
	OutCommit.Element = Element;
	OutCommit.RawDamage = ComputeDamage(*Step, Stats, Element, bCritical);
	OutCommit.RangeCm = Step->RangeCm > 0.0f ? Step->RangeCm : Definition.RangeCm;
	OutCommit.ArcDegrees = Step->ArcDegrees > 0.0f ? Step->ArcDegrees : Definition.ArcDegrees;
	OutCommit.ProjectileCount = FMath::Max(Definition.ProjectileCount, Step->ProjectileCount);
	OutCommit.SpreadDegrees = Step->SpreadDegrees > 0.0f ? Step->SpreadDegrees : Definition.SpreadDegrees;
	OutCommit.ExplosionRadiusCm =
	    Step->ExplosionRadiusCm > 0.0f ? Step->ExplosionRadiusCm : Definition.ExplosionRadiusCm;
	OutCommit.MovementCm = Step->MovementCm;
	OutCommit.OuterRingStartFraction = Step->OuterRingStartFraction;
	OutCommit.OuterRingBonusMultiplier = Step->OuterRingBonusMultiplier;
	OutCommit.BehaviorDurationSeconds = ScaledDuration;
	OutCommit.bInvulnerable = Step->bInvulnerable;
	OutCommit.bCritical = bCritical;

	++SuccessfulAttackCount;
	NextStepCursor = (NextStepCursor + 1) % Definition.AttackSteps.Num();
	return true;
}

EReEchoElement FReEchoWeaponLogic::ResolveElement()
{
	if (Definition.bUsesCyclingElement)
	{
		const EReEchoElement Element = ResolveCyclingElement(NextElementIndex);
		NextElementIndex = (NextElementIndex + 1) % 12;
		return Element;
	}
	return ResolveDamageChannel(Definition.DamageChannelId, SuccessfulAttackCount + 1);
}

float FReEchoWeaponLogic::ComputeDamage(const FReEchoWeaponStepDefinition& Step,
                                        const FReEchoStatBlock& Stats,
                                        const EReEchoElement Element,
                                        bool& bOutCritical)
{
	const float DamageCoefficient =
	    Step.DamageCoefficient > 0.0f ? Step.DamageCoefficient : Definition.DamageCoefficient;
	float Damage = Element == EReEchoElement::None ? Stats.PhysicalAttack * DamageCoefficient
	                                               : Stats.ElementalAttack * DamageCoefficient;

	bOutCritical = false;
	if (Stats.RoleId == TEXT("Hunter"))
	{
		CriticalAccumulator += FMath::Clamp(Stats.CriticalRate, 0.0f, 1.0f);
		if (CriticalAccumulator >= 1.0f)
		{
			CriticalAccumulator -= 1.0f;
			Damage *= 1.0f + FMath::Max(0.0f, Stats.CriticalEffect);
			bOutCritical = true;
		}
	}
	return FMath::Max(0.0f, Damage);
}
