#include "Data/ReEchoBossPhase3Config.h"

bool UReEchoBossPhase3Config::ApplyTo(const FName EnemyId, FReEchoEnemyDefinition& InOutDefinition) const
{
	if (EnemyId != BossEnemyId || InOutDefinition.Archetype != EReEchoEnemyArchetype::Boss ||
	    ExistingAbilityMaxPhaseIndex < 1 || TriggerSeconds < 0.0f || PhaseMaxHealth <= 0.0f || AbilityId.IsNone() ||
	    Damage < 0.0f || WindupSeconds < 0.0f || ActiveSeconds < 0.0f || RecoverySeconds < 0.0f ||
	    CooldownSeconds < 0.0f || MaxRangeCm <= 0.0f || RadiusCm <= 0.0f || TeleportOffsetCm <= 0.0f || ComboMin < 1 ||
	    ComboMax < ComboMin || ComboMax > 3 || !FMath::IsFinite(PreviousPhasesHealthMultiplier) ||
	    PreviousPhasesHealthMultiplier < 1.0f || SpawnPlanRepetitions < 1 || SpawnPlanRepetitions > 20 ||
	    OpeningStrikeCount < ComboMin || OpeningStrikeCount > ComboMax || OpeningRepulseDistanceCm < 0.0f ||
	    !FMath::IsFinite(OpeningRepulseDistanceCm) || !FMath::IsFinite(OpeningRepulseSeconds) ||
	    OpeningRepulseSeconds <= 0.0f)
	{
		return false;
	}

	for (FReEchoEnemyAbilityDefinition& Ability : InOutDefinition.Abilities)
	{
		Ability.MinPhaseIndex = 1;
		Ability.MaxPhaseIndex = ExistingAbilityMaxPhaseIndex;
		Ability.ComboMin = 1;
		Ability.ComboMax = 1;
	}
	FReEchoEnemyAbilityDefinition* ExistingSkill03 = InOutDefinition.Abilities.FindByPredicate(
	    [this](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == AbilityId;
	    });
	if (!ExistingSkill03 || ExistingSkill03->BehaviorId != TEXT("Boss.BlinkSlam"))
	{
		return false;
	}
	FReEchoEnemyAbilityDefinition& Phase3Ability = *ExistingSkill03;
	Phase3Ability.Damage = Damage;
	Phase3Ability.WindupSeconds = WindupSeconds;
	Phase3Ability.ActiveSeconds = ActiveSeconds;
	Phase3Ability.RecoverySeconds = RecoverySeconds;
	Phase3Ability.CooldownSeconds = CooldownSeconds;
	Phase3Ability.MaxRangeCm = MaxRangeCm;
	Phase3Ability.RadiusCm = RadiusCm;
	Phase3Ability.TeleportOffsetCm = TeleportOffsetCm;
	Phase3Ability.TargetingMode = EReEchoBossTargetingMode::LockedLocation;
	Phase3Ability.LockTiming = EReEchoBossLockTiming::WindupStarted;
	Phase3Ability.bMovementDuringCast = false;
	Phase3Ability.MinPhaseIndex = 1;
	Phase3Ability.MaxPhaseIndex = 3;
	Phase3Ability.ComboMin = ComboMin;
	Phase3Ability.ComboMax = ComboMax;
	Phase3Ability.bEnabled = true;
	FReEchoEnemyAbilityDefinition GroundTriple = Phase3Ability;
	GroundTriple.Id = TEXT("M_SHEEP_GroundTriple");
	GroundTriple.bGroundedSlam = true;
	GroundTriple.MinPhaseIndex = 3;
	GroundTriple.MaxPhaseIndex = 3;
	GroundTriple.ComboMin = 3;
	GroundTriple.ComboMax = 3;
	GroundTriple.SequenceOrder = Phase3Ability.SequenceOrder + 1;
	InOutDefinition.Abilities.RemoveAll(
	    [](const FReEchoEnemyAbilityDefinition& Ability)
	    {
		    return Ability.Id == TEXT("M_SHEEP_GroundTriple");
	    });
	InOutDefinition.Abilities.Add(MoveTemp(GroundTriple));
	InOutDefinition.Abilities.Sort(
	    [](const FReEchoEnemyAbilityDefinition& Left, const FReEchoEnemyAbilityDefinition& Right)
	    {
		    return Left.SequenceOrder < Right.SequenceOrder;
	    });

	InOutDefinition.BossPhases.RemoveAll(
	    [](const FReEchoBossPhaseDefinition& Phase)
	    {
		    return Phase.PhaseIndex == 3;
	    });
	FReEchoBossPhaseDefinition Phase3;
	Phase3.Id = TEXT("M_SHEEP_Phase3");
	Phase3.PhaseIndex = 3;
	Phase3.TriggerSeconds = TriggerSeconds;
	Phase3.PhysicalAttackMultiplier = 1.5f;
	Phase3.ElementalAttackMultiplier = 1.5f;
	Phase3.AttackSpeedMultiplier = 1.4f;
	Phase3.MovementSpeedMultiplier = 1.0f;
	Phase3.RefillHealthPolicy = EReEchoBossRefillHealthPolicy::RefillToMaximum;
	Phase3.PhaseMaxHealth = PhaseMaxHealth;
	Phase3.PreviousPhasesHealthMultiplier = PreviousPhasesHealthMultiplier;
	Phase3.OpeningAbilityId = TEXT("M_SHEEP_GroundTriple");
	Phase3.OpeningStrikeCount = OpeningStrikeCount;
	Phase3.OpeningRepulseDistanceCm = OpeningRepulseDistanceCm;
	Phase3.OpeningRepulseRadiusCm = FMath::Max(RadiusCm, SacrificeRadiusCm);
	Phase3.OpeningRepulseSeconds = OpeningRepulseSeconds;
	Phase3.bEnabled = true;
	InOutDefinition.BossPhases.Add(MoveTemp(Phase3));
	InOutDefinition.BossPhases.Sort(
	    [](const FReEchoBossPhaseDefinition& Left, const FReEchoBossPhaseDefinition& Right)
	    {
		    return Left.PhaseIndex < Right.PhaseIndex;
	    });
	return true;
}
