#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoAttackControllerComponent.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoElementRuntime.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatAttackModeCommandTest,
                                 "ReEcho.Combat.AttackModeCommand",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatAttackModeCommandTest::RunTest(const FString& Parameters)
{
	UReEchoAttackControllerComponent* Controller = NewObject<UReEchoAttackControllerComponent>();
	TestEqual(TEXT("Automatic attack is the default"), Controller->GetAttackMode(), EReEchoAttackMode::Automatic);
	Controller->SetAttackMode(EReEchoAttackMode::Manual);
	TestEqual(TEXT("The typed command changes mode"), Controller->GetAttackMode(), EReEchoAttackMode::Manual);
	TestFalse(TEXT("Changing mode releases manual input"), Controller->IsManualHeld());
	TestFalse(TEXT("Changing mode releases automatic input"), Controller->IsAutomaticHeld());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatantSnapshotTest,
                                 "ReEcho.Combat.CombatantSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatantSnapshotTest::RunTest(const FString& Parameters)
{
	UReEchoCombatantComponent* Combatant = NewObject<UReEchoCombatantComponent>();
	FReEchoStatBlock Stats;
	Stats.HpMax = 125.0f;
	Stats.PhysicalAttack = 17.0f;
	Stats.AttackSpeed = 1.25f;
	Combatant->InitializeFromStats(Stats, true);

	const FReEchoCombatantSnapshot Snapshot = Combatant->GetSnapshot();
	TestEqual(TEXT("Snapshot identifies its combatant"), Snapshot.Combatant.Get(), Combatant);
	TestEqual(TEXT("Snapshot captures current health"), Snapshot.CurrentHealth, 125.0f);
	TestEqual(TEXT("Snapshot captures maximum health"), Snapshot.MaximumHealth, 125.0f);
	TestEqual(TEXT("Snapshot captures combat stats"), Snapshot.Stats.PhysicalAttack, 17.0f);
	TestTrue(TEXT("Snapshot captures alive state"), Snapshot.bAlive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementCleanseCommandTest,
                                 "ReEcho.Combat.ElementCleanseCommand",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementCleanseCommandTest::RunTest(const FString& Parameters)
{
	AActor* Owner = NewObject<AActor>();
	UReEchoCombatantComponent* Combatant = NewObject<UReEchoCombatantComponent>(Owner);
	UReEchoCombatEventsComponent* Events = NewObject<UReEchoCombatEventsComponent>(Owner);
	Owner->AddInstanceComponent(Combatant);
	Owner->AddInstanceComponent(Events);
	FReEchoElementState& State = Combatant->EditElementStateForTests();
	State.Attached = EReEchoElement::Grass;
	State.ImmunityUntil = 8.0f;
	State.ActiveStatusUntilSeconds.Add(TEXT("Z_Burn"), 13.0f);
	State.ActiveStatusUntilSeconds.Add(TEXT("Unrelated.Status"), 25.0f);
	State.bBurnActive = true;
	State.BurnTickDamage = 7.0f;
	State.BurnNextTickTimeSeconds = 11.0f;
	State.BurnSourceLocation = FVector(10.0f, 20.0f, 0.0f);
	State.bEnhancedNextReaction = true;
	State.EnhancementMultiplier = 2.0f;
	State.BlockedAttachment = EReEchoElement::Water;

	FReEchoElementCleanseCommand Command;
	Command.CurrentTimeSeconds = 10.0f;
	Command.ImmunityDurationSeconds = 1.0f;
	const FReEchoElementCleanseResult Result = Combatant->ExecuteElementCleanse(Command);
	TestTrue(TEXT("Valid cleanse command succeeds"), Result.bSucceeded);
	TestTrue(TEXT("First cleanse changes state"), Result.bStateChanged);
	TestTrue(TEXT("Cleanse reports removed attachment"), Result.bClearedAttachment);
	TestTrue(TEXT("Cleanse reports removed burn"), Result.bClearedBurn);
	TestEqual(TEXT("Cleanse reports immunity end"), Result.ImmunityUntil, 11.0f);
	TestEqual(TEXT("Attachment is cleared"), State.Attached, EReEchoElement::None);
	TestFalse(TEXT("Burn is inactive"), State.bBurnActive);
	TestFalse(TEXT("Burn status is removed"), State.ActiveStatusUntilSeconds.Contains(TEXT("Z_Burn")));
	TestEqual(TEXT("Burn damage payload is cleared"), State.BurnTickDamage, 0.0f);
	TestEqual(TEXT("Burn schedule payload is cleared"), State.BurnNextTickTimeSeconds, 0.0f);
	TestTrue(TEXT("Burn source payload is cleared"), State.BurnSourceLocation.IsNearlyZero());
	TestEqual(TEXT("Deterministic immunity uses caller time"), State.ImmunityUntil, 11.0f);
	TestEqual(TEXT("Immunity status mirrors immunity end"),
	          State.ActiveStatusUntilSeconds.FindRef(TEXT("Z_Elemental_Immunity")),
	          11.0f);
	TestEqual(
	    TEXT("Unrelated statuses remain"), State.ActiveStatusUntilSeconds.FindRef(TEXT("Unrelated.Status")), 25.0f);
	TestTrue(TEXT("Reaction enhancement remains"), State.bEnhancedNextReaction);
	TestEqual(TEXT("Reaction enhancement multiplier remains"), State.EnhancementMultiplier, 2.0f);
	TestEqual(TEXT("Attachment block remains"), State.BlockedAttachment, EReEchoElement::Water);
	TestEqual(
	    TEXT("Changed cleanse publishes one element-state event"), Events->GetElementStatePublishCountForTests(), 1);

	const FReEchoElementCleanseResult Repeated = Combatant->ExecuteElementCleanse(Command);
	TestTrue(TEXT("Idempotent valid command still succeeds"), Repeated.bSucceeded);
	TestFalse(TEXT("Idempotent command reports no state change"), Repeated.bStateChanged);
	TestFalse(TEXT("Idempotent command clears no attachment"), Repeated.bClearedAttachment);
	TestFalse(TEXT("Idempotent command clears no burn"), Repeated.bClearedBurn);
	TestEqual(TEXT("No-op cleanse does not republish element state"), Events->GetElementStatePublishCountForTests(), 1);

	FReEchoElementCleanseCommand InvalidCommand;
	InvalidCommand.CurrentTimeSeconds = 12.0f;
	InvalidCommand.ImmunityDurationSeconds = 0.0f;
	const FReEchoElementCleanseResult Invalid = Combatant->ExecuteElementCleanse(InvalidCommand);
	TestFalse(TEXT("Non-positive immunity duration rejects the command"), Invalid.bSucceeded);
	TestFalse(TEXT("Rejected command does not change state"), Invalid.bStateChanged);
	TestEqual(TEXT("Rejected command preserves immunity"), State.ImmunityUntil, 11.0f);
	TestEqual(
	    TEXT("Rejected command does not publish element state"), Events->GetElementStatePublishCountForTests(), 1);

	const FReEchoCombatantSnapshot Snapshot = Combatant->GetSnapshot();
	UReEchoCombatantComponent* Restored = NewObject<UReEchoCombatantComponent>();
	Restored->RestoreElementState(Snapshot.ElementState);
	TestEqual(TEXT("Snapshot carries cleanse immunity"), Restored->GetElementState().ImmunityUntil, 11.0f);
	TestFalse(TEXT("Snapshot carries cleansed burn"), Restored->GetElementState().bBurnActive);

	TSharedRef<FReEchoElementRuleSet> Rules = MakeShared<FReEchoElementRuleSet>();
	FReEchoElementRuleDefinition Flame;
	Flame.ElementId = TEXT("Flame");
	Flame.Element = EReEchoElement::Flame;
	Flame.bAttachment = true;
	Flame.bEnabled = true;
	Rules->Elements.Add(EReEchoElement::Flame, Flame);
	ReEchoElementRuntime::PublishRuleSet(Rules);
	FReEchoElementState RuntimeState = Snapshot.ElementState;
	const FReEchoElementHitResult Blocked =
	    ReEchoElementRuntime::ResolveHit(RuntimeState, EReEchoElement::Flame, 5.0f, 1.0f, 10.999f);
	TestTrue(TEXT("Cleanse immunity blocks element hits before the boundary"), Blocked.bBlockedByImmunity);
	const FReEchoElementHitResult AtBoundary =
	    ReEchoElementRuntime::ResolveHit(RuntimeState, EReEchoElement::Flame, 5.0f, 1.0f, 11.0f);
	TestFalse(TEXT("Cleanse immunity expires exactly at the boundary"), AtBoundary.bBlockedByImmunity);
	TestEqual(
	    TEXT("Element attachment resumes at the immunity boundary"), RuntimeState.Attached, EReEchoElement::Flame);
	ReEchoElementRuntime::ClearRuleSetForTests();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoAttackIdentitySourceLifetimeTest,
                                 "ReEcho.Combat.AttackIdentity.SourceLifetime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoAttackIdentitySourceLifetimeTest::RunTest(const FString& Parameters)
{
	FReEchoAttackIdentity EmptyIdentity;
	TestFalse(TEXT("Unassigned identity is invalid"), EmptyIdentity.IsValid());

	AActor* Source = NewObject<AActor>();
	FReEchoAttackIdentity Identity;
	Identity.Source = Source;
	Identity.Sequence = 7;
	const FReEchoAttackIdentity IdentityCopy = Identity;
	TestTrue(TEXT("Live source makes the identity valid"), Identity.IsValid());
	TestTrue(TEXT("Live source is available for optional feedback"), Identity.HasLiveSource());
	TestEqual(TEXT("Weak source resolves while live"), Identity.Source.Get(), Source);

	Source->MarkAsGarbage();
	TestTrue(TEXT("Destroyed source does not erase the assigned attack identity"), Identity.IsValid());
	TestTrue(TEXT("Identity comparison remains stable after source destruction"), Identity == IdentityCopy);
	TestFalse(TEXT("Destroyed source is unavailable for optional feedback"), Identity.HasLiveSource());
	TestNull(TEXT("Destroyed source cannot be dereferenced by delayed hit resolution"), Identity.Source.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuneTimedStatusRuntimeTest,
                                 "ReEcho.Combat.Runes.TimedStatusAndIndependentStacks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuneTimedStatusRuntimeTest::RunTest(const FString& Parameters)
{
	UReEchoCombatantComponent* Combatant = NewObject<UReEchoCombatantComponent>();
	FReEchoStatBlock Stats;
	Stats.HpMax = 100.0f;
	Stats.HpPoint = 100.0f;
	Stats.AttackSpeed = 1.0f;
	Stats.MovementSpeed = 100.0f;
	Combatant->InitializeFromStats(Stats, true);

	FReEchoTimedStatusCommand Stun;
	Stun.StatusId = TEXT("Z_Vertigo");
	Stun.CurrentTimeSeconds = 0.0f;
	Stun.DurationSeconds = 1.0f;
	TestTrue(TEXT("Vertigo command is accepted"), Combatant->ApplyTimedStatus(Stun));
	TestTrue(TEXT("Vertigo disables actions before expiry"), Combatant->IsActionDisabled(0.999f));
	TestFalse(TEXT("Vertigo expires exactly at its boundary"), Combatant->IsActionDisabled(1.0f));
	Combatant->GrantTimedInvulnerability(0.0f, 0.5f);
	TestTrue(TEXT("Group-hit invulnerability is active before its boundary"), Combatant->IsTimedInvulnerable(0.499f));
	TestFalse(TEXT("Group-hit invulnerability expires exactly at its boundary"), Combatant->IsTimedInvulnerable(0.5f));

	FReEchoTimedStatusCommand Bleed;
	Bleed.StatusId = TEXT("Z_Bleeding");
	Bleed.CurrentTimeSeconds = 0.0f;
	Bleed.DurationSeconds = 3.0f;
	Bleed.DamagePerTickMaxHealthFraction = 0.005f;
	TestTrue(TEXT("First bleeding stack is accepted"), Combatant->ApplyTimedStatus(Bleed));
	TestTrue(TEXT("Second bleeding stack is independent"), Combatant->ApplyTimedStatus(Bleed));
	Combatant->AdvanceTimedRuneEffectsForTests(1.0f);
	TestTrue(TEXT("Two stacks each deal exactly 0.5% max HP per second"),
	         FMath::IsNearlyEqual(Combatant->CurrentHealth, 99.0f));

	FReEchoTimedStatusCommand LaterBleed = Bleed;
	LaterBleed.CurrentTimeSeconds = 1.5f;
	TestTrue(TEXT("Later bleeding stack receives its own expiry"), Combatant->ApplyTimedStatus(LaterBleed));
	Combatant->AdvanceTimedRuneEffectsForTests(3.0f);
	TestTrue(TEXT("Earlier stacks tick three times while later stack ticks once"),
	         FMath::IsNearlyEqual(Combatant->CurrentHealth, 96.5f));
	TestTrue(TEXT("Later stack remains after earlier stacks expire"),
	         Combatant->GetElementState().ActiveStatusUntilSeconds.Contains(TEXT("Z_Bleeding")));
	Combatant->AdvanceTimedRuneEffectsForTests(4.5f);
	TestTrue(TEXT("Later stack ticks through its own three-second boundary"),
	         FMath::IsNearlyEqual(Combatant->CurrentHealth, 95.5f));
	TestFalse(TEXT("Bleeding status clears after the final independent stack expires"),
	          Combatant->GetElementState().ActiveStatusUntilSeconds.Contains(TEXT("Z_Bleeding")));

	Combatant->AddTransientStatModifier(TEXT("Rune.Attack"), 0.01f, 0.0f, 5.0f, 30);
	Combatant->AddTransientStatModifier(TEXT("Rune.Attack"), 0.01f, 0.0f, 5.0f, 30);
	TestEqual(TEXT("Transient rune stat stacks are tracked independently"),
	          Combatant->GetTransientStatStackCount(TEXT("Rune.Attack")),
	          2);
	TestTrue(TEXT("Transient percentage layers add against the unmodified base instead of compounding"),
	         FMath::IsNearlyEqual(Combatant->Stats.AttackSpeed, 1.02f, 0.0001f));
	Combatant->AdvanceTimedRuneEffectsForTests(5.0f);
	TestEqual(TEXT("All independently expired transient stacks are removed"),
	          Combatant->GetTransientStatStackCount(TEXT("Rune.Attack")),
	          0);
	TestTrue(TEXT("Removing transient stacks restores the base stat"),
	         FMath::IsNearlyEqual(Combatant->Stats.AttackSpeed, 1.0f, 0.0001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatFactionRelationsTest,
                                 "ReEcho.Combat.FactionRelations",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatFactionRelationsTest::RunTest(const FString& Parameters)
{
	using namespace ReEchoCombatRelations;

	TestFalse(TEXT("Player-side attacks cannot damage player-side targets"),
	          CanDamage(EReEchoCombatFaction::PlayerSide, EReEchoCombatFaction::PlayerSide));
	TestTrue(TEXT("Player-side attacks can damage enemy-side targets"),
	         CanDamage(EReEchoCombatFaction::PlayerSide, EReEchoCombatFaction::EnemySide));
	TestFalse(TEXT("Enemy-side attacks cannot damage enemy-side targets"),
	          CanDamage(EReEchoCombatFaction::EnemySide, EReEchoCombatFaction::EnemySide));
	TestTrue(TEXT("Enemy-side attacks can damage player-side targets"),
	         CanDamage(EReEchoCombatFaction::EnemySide, EReEchoCombatFaction::PlayerSide));
	TestTrue(TEXT("Authored same-faction damage exceptions remain possible"),
	         CanDamage(EReEchoCombatFaction::EnemySide, EReEchoCombatFaction::EnemySide, true));
	TestTrue(TEXT("Unaligned legacy callers retain compatibility"),
	         CanDamage(EReEchoCombatFaction::Unaligned, EReEchoCombatFaction::PlayerSide));
	return true;
}

#endif
