#include "Misc/AutomationTest.h"

#include "Algo/Reverse.h"
#include "Diagnostics/ReEchoBuildTrace.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FReEchoBuildSnapshot MakeTraceBuild()
{
	FReEchoBuildSnapshot Build;
	Build.CharacterId = TEXT("J_DIAMOND");
	Build.WeaponId = TEXT("W_J_09");
	Build.WeaponDataRevision = 7;
	Build.WeaponDomainRevision = TEXT("weapon-rev");
	Build.bHasEquipmentBase = true;
	Build.EquipmentBaseStats.PhysicalAttack = 12.5f;
	Build.EquipmentBaseRuleFlags.Add(TEXT("Base.B"), TEXT("2"));
	Build.EquipmentBaseRuleFlags.Add(TEXT("Base.A"), TEXT("1"));
	FReEchoEquippedPartSnapshot Muzzle;
	Muzzle.SlotTypeId = TEXT("Muzzle");
	Muzzle.PartId = TEXT("P_GUN_TRIPLESPREAD_MUZZLE");
	FReEchoEquippedPartSnapshot Action;
	Action.SlotTypeId = TEXT("Action");
	Action.PartId = TEXT("P_GUN_HITSHARD_GUNACTION");
	Build.EquippedParts = {Muzzle, Action};
	Build.CardState.DomainRevision = TEXT("card-rev");
	Build.CardState.OwnedCardIds = {TEXT("G_2_01"), TEXT("G_1_03"), TEXT("G_1_03")};
	Build.CardState.Runtime.RandomSequence = 11;
	Build.CardState.Runtime.ActiveEncounterIndex = 3;
	Build.CardState.Runtime.ReactionCount = 4;
	Build.CardState.Runtime.PlayerDamageMultiplier = 1.25f;
	Build.CardState.Runtime.EchoDamageMultiplier = 0.75f;
	Build.CardState.Runtime.DistinctReactionIds = {TEXT("WaterGrass"), TEXT("FireGrass")};
	Build.CardState.Runtime.MasteredWeaponIds = {TEXT("W_J_09"), TEXT("W_J_01")};
	Build.CardState.Runtime.ConductAffectedSpawnIndices = {9, 2};
	Build.CardState.Runtime.PlayerKillCountByEnemyId.Add(TEXT("Enemy.Rabbit"), 2);
	Build.CardState.Runtime.PlayerKillCountByEnemyId.Add(TEXT("Enemy.Fox"), 1);
	Build.Stats.PhysicalAttack = 18.0f;
	Build.Stats.CriticalRate = 0.45f;
	Build.Stats.CriticalEffect = 0.9f;
	Build.RuleFlags.Add(TEXT("Rule.Z"), TEXT("last"));
	Build.RuleFlags.Add(TEXT("Rule.A"), TEXT("first"));
	return Build;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBuildTraceDeterminismTest,
                                 "ReEcho.Diagnostics.BuildTrace.Determinism",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBuildTraceDeterminismTest::RunTest(const FString& Parameters)
{
	const FReEchoBuildSnapshot First = MakeTraceBuild();
	FReEchoBuildSnapshot Reordered = First;
	Algo::Reverse(Reordered.EquippedParts);
	Algo::Reverse(Reordered.CardState.OwnedCardIds);
	Algo::Reverse(Reordered.CardState.Runtime.DistinctReactionIds);
	Algo::Reverse(Reordered.CardState.Runtime.MasteredWeaponIds);
	Algo::Reverse(Reordered.CardState.Runtime.ConductAffectedSpawnIndices);
	Reordered.RuleFlags.Reset();
	Reordered.RuleFlags.Add(TEXT("Rule.A"), TEXT("first"));
	Reordered.RuleFlags.Add(TEXT("Rule.Z"), TEXT("last"));
	Reordered.EquipmentBaseRuleFlags.Reset();
	Reordered.EquipmentBaseRuleFlags.Add(TEXT("Base.A"), TEXT("1"));
	Reordered.EquipmentBaseRuleFlags.Add(TEXT("Base.B"), TEXT("2"));
	Reordered.CardState.Runtime.PlayerKillCountByEnemyId.Reset();
	Reordered.CardState.Runtime.PlayerKillCountByEnemyId.Add(TEXT("Enemy.Fox"), 1);
	Reordered.CardState.Runtime.PlayerKillCountByEnemyId.Add(TEXT("Enemy.Rabbit"), 2);

	TestEqual(TEXT("Equivalent unordered build content has one canonical form"),
	          ReEchoBuildTrace::BuildCanonical(First),
	          ReEchoBuildTrace::BuildCanonical(Reordered));
	TestEqual(TEXT("Equivalent unordered build content has one fingerprint"),
	          ReEchoBuildTrace::ComputeFingerprint(First),
	          ReEchoBuildTrace::ComputeFingerprint(Reordered));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBuildTraceMutationTest,
                                 "ReEcho.Diagnostics.BuildTrace.Mutations",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBuildTraceMutationTest::RunTest(const FString& Parameters)
{
	const FReEchoBuildSnapshot Base = MakeTraceBuild();
	const FString BaseFingerprint = ReEchoBuildTrace::ComputeFingerprint(Base);
	const auto ExpectFingerprintChange = [this, &BaseFingerprint](const TCHAR* What, const FReEchoBuildSnapshot& Changed)
	{
		TestTrue(What, ReEchoBuildTrace::ComputeFingerprint(Changed) != BaseFingerprint);
	};

	FReEchoBuildSnapshot CardChanged = Base;
	CardChanged.CardState.OwnedCardIds.Add(TEXT("G_1_03"));
	ExpectFingerprintChange(TEXT("Card stack changes the fingerprint"), CardChanged);

	FReEchoBuildSnapshot RuneChanged = Base;
	RuneChanged.EquippedParts[0].PartId = TEXT("P_OTHER");
	ExpectFingerprintChange(TEXT("Rune changes the fingerprint"), RuneChanged);

	FReEchoBuildSnapshot RuleChanged = Base;
	RuleChanged.RuleFlags[TEXT("Rule.A")] = TEXT("changed");
	ExpectFingerprintChange(TEXT("Rule changes the fingerprint"), RuleChanged);

	FReEchoBuildSnapshot StatChanged = Base;
	StatChanged.Stats.PhysicalAttack += 1.0f;
	ExpectFingerprintChange(TEXT("Effective stat changes the fingerprint"), StatChanged);

	FReEchoBuildSnapshot RuntimeChanged = Base;
	RuntimeChanged.CardState.Runtime.PlayerDamageMultiplier += 0.1f;
	ExpectFingerprintChange(TEXT("Damage-affecting runtime changes the fingerprint"), RuntimeChanged);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBuildTraceSummaryTest,
                                 "ReEcho.Diagnostics.BuildTrace.Summary",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBuildTraceSummaryTest::RunTest(const FString& Parameters)
{
	const FString Summary = ReEchoBuildTrace::BuildSummary(MakeTraceBuild());
	TestTrue(TEXT("Summary contains a fingerprint"), Summary.Contains(TEXT("fingerprint=")));
	TestTrue(TEXT("Summary contains exact character identity"), Summary.Contains(TEXT("character=J_DIAMOND")));
	TestTrue(TEXT("Summary contains exact weapon identity"), Summary.Contains(TEXT("weapon=W_J_09")));
	TestTrue(TEXT("Summary collapses duplicate cards into stacks"), Summary.Contains(TEXT("G_1_03x2")));
	TestTrue(TEXT("Summary lists rune slot and identity"),
	         Summary.Contains(TEXT("Muzzle:P_GUN_TRIPLESPREAD_MUZZLE")));
	TestTrue(TEXT("Summary includes effective combat stats"), Summary.Contains(TEXT("physical=18.000000")));
	TestTrue(TEXT("Summary includes key runtime counters"), Summary.Contains(TEXT("reactions=4")));
	TestTrue(TEXT("Summary includes weapon and card revisions"),
	         Summary.Contains(TEXT("weaponRevision=7")) && Summary.Contains(TEXT("cardDomain=card-rev")));
	return true;
}

#endif
