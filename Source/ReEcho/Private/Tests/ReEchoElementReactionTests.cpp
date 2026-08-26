#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace
{
FString AssembleElementCsvFixture(const TCHAR* FixtureName)
{
	const FString SourceDataDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
	const FString FixtureDirectory =
	    FPaths::Combine(SourceDataDirectory, TEXT("TestFixtures"), TEXT("CsvRuntime"), FixtureName);
	const FString AssembledDirectory =
	    FPaths::Combine(FPaths::ProjectSavedDir(),
	                    TEXT("Automation"),
	                    TEXT("CsvRuntime"),
	                    FString::Printf(TEXT("%s_%s"), FixtureName, *FGuid::NewGuid().ToString(EGuidFormats::Digits)));

	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*AssembledDirectory, true);

	TArray<FString> ProductionCsvFiles;
	FileManager.FindFiles(ProductionCsvFiles, *FPaths::Combine(SourceDataDirectory, TEXT("*.csv")), true, false);
	for (const FString& FileName : ProductionCsvFiles)
	{
		FileManager.Copy(*FPaths::Combine(AssembledDirectory, FileName),
		                 *FPaths::Combine(SourceDataDirectory, FileName));
	}

	TArray<FString> OverrideCsvFiles;
	FileManager.FindFiles(OverrideCsvFiles, *FPaths::Combine(FixtureDirectory, TEXT("*.csv")), true, false);
	for (const FString& FileName : OverrideCsvFiles)
	{
		FileManager.Copy(*FPaths::Combine(AssembledDirectory, FileName), *FPaths::Combine(FixtureDirectory, FileName));
	}
	return AssembledDirectory;
}

struct FReEchoElementWorldFixture
{
	UWorld* World = nullptr;

	FReEchoElementWorldFixture()
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("ReEchoElementTestWorld"));
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->SetShouldTick(true);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}

	~FReEchoElementWorldFixture()
	{
		if (World)
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}

	AReEchoEnemyActor* SpawnEnemy(const FVector& Location,
	                              const int32 SpawnIndex,
	                              const float HpMax = 1000.0f,
	                              const EReEchoEnemyKind Kind = EReEchoEnemyKind::Grunt)
	{
		FActorSpawnParameters SpawnParams;
		AReEchoEnemyActor* Enemy = World->SpawnActor<AReEchoEnemyActor>(Location, FRotator::ZeroRotator, SpawnParams);
		Enemy->Configure(Kind, SpawnIndex);
		FReEchoStatBlock Stats;
		Stats.HpMax = HpMax;
		Stats.HpPoint = HpMax;
		Stats.Block = 0;
		Enemy->GetCombatantComponent()->BindToAbilitySystem(Enemy->GetAbilitySystemComponent());
		Enemy->GetCombatantComponent()->InitializeFromStats(Stats, true);
		return Enemy;
	}

	AActor* SpawnSource(const float ElementalAttack, const float EchoEfficiency)
	{
		AActor* Source = World->SpawnActor<AActor>();
		UReEchoCombatantComponent* SourceCombatant =
		    NewObject<UReEchoCombatantComponent>(Source, TEXT("SourceCombatant"));
		Source->AddInstanceComponent(SourceCombatant);
		SourceCombatant->RegisterComponent();
		FReEchoStatBlock Stats;
		Stats.ElementalAttack = ElementalAttack;
		Stats.EchoEfficiency = EchoEfficiency;
		SourceCombatant->InitializeFromStats(Stats, true);
		return Source;
	}

	FReEchoElementHitContext
	MakeContext(const float ElementalAttack, const float EchoEfficiency = 1.0f, const float ReactionEfficiency = 1.0f)
	{
		FReEchoElementHitContext Context;
		Context.SourceElementalAttack = ElementalAttack;
		Context.SourceEchoEfficiency = EchoEfficiency;
		Context.ReactionEfficiency = ReactionEfficiency;
		Context.SourceLocation = FVector::ZeroVector;
		return Context;
	}

	FReEchoAttackIdentity MakeAttack(AActor* Source, const int64 Sequence = 1)
	{
		FReEchoAttackIdentity Attack;
		Attack.Source = Source;
		Attack.Sequence = Sequence;
		return Attack;
	}

	void AdvanceWorldTime(const float Seconds)
	{
		const double PreviousTimeSeconds = World->GetTimeSeconds();
		const double ExpectedTimeSeconds = PreviousTimeSeconds + Seconds;
		World->Tick(ELevelTick::LEVELTICK_All, Seconds);
		if (!FMath::IsNearlyEqual(World->GetTimeSeconds(), ExpectedTimeSeconds, 0.001))
		{
			World->TimeSeconds = ExpectedTimeSeconds;
		}
	}

	void AdvanceTimeOnly(const float Seconds)
	{
		World->TimeSeconds = World->GetTimeSeconds() + Seconds;
	}

	void Advance(const float Seconds)
	{
		AdvanceWorldTime(Seconds);
		for (TActorIterator<AReEchoEnemyActor> It(World); It; ++It)
		{
			ReEchoElementReaction::TickElementStatuses(**It, World->GetTimeSeconds());
		}
	}
};

float EnemyHealth(const AReEchoEnemyActor* Enemy)
{
	return Enemy->GetCombatantComponent()->CurrentHealth;
}

template <typename ElementType> ElementType* ResolveWeak(const TWeakObjectPtr<ElementType>& WeakObject)
{
	return WeakObject.Get();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementReactionTest,
                                 "ReEcho.Combat.ElementReactions",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementReactionTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads for element reactions"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	TestTrue(TEXT("Lightning is now a combat element"),
	         ReEchoElementReaction::IsCombatElement(EReEchoElement::Lightning));

	FReEchoElementState State;
	FReEchoElementHitResult Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 10.0f);
	TestFalse(TEXT("Trigger-only flame hit does not attach by itself"), Result.bTriggeredReaction);
	TestEqual(TEXT("Trigger element does not become attachment"), State.Attached, EReEchoElement::None);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f);
	TestFalse(TEXT("First grass hit only attaches"), Result.bTriggeredReaction);
	TestEqual(TEXT("Grass becomes attached"), State.Attached, EReEchoElement::Grass);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 10.0f, 1.0f, 10.0f);
	TestTrue(TEXT("Flame over grass triggers burn"), Result.bTriggeredReaction);
	TestEqual(TEXT("Burn reaction id comes from CSV"), Result.ReactionId, FName(TEXT("Y_ER_F_G")));
	TestEqual(TEXT("Burn pure result does not fold DOT into immediate damage"), Result.Damage, 0.0f);
	TestEqual(TEXT("Burn formula comes from CSV"), Result.FormulaId, FName(TEXT("Element.ElementAttackDot")));
	TestEqual(TEXT("Burn status is applied"), Result.AppliedStatusId, FName(TEXT("Z_Burn")));
	TestEqual(TEXT("Reaction consumes attachment"), State.Attached, EReEchoElement::None);
	TestTrue(TEXT("Reaction starts elemental immunity"), State.ImmunityUntil > 10.0f);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Water, 8.0f, 1.0f, 11.0f);
	TestTrue(TEXT("Elemental immunity blocks immediate attachment"), Result.bBlockedByImmunity);
	TestEqual(TEXT("Blocked hit keeps attachment clear"), State.Attached, EReEchoElement::None);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 40.0f);
	TestEqual(TEXT("Vaporize reaction id is ordered flame over water"), Result.ReactionId, FName(TEXT("Y_ER_F_W")));
	TestEqual(TEXT("Vaporize pure result leaves squared damage to world execution"), Result.Damage, 0.0f);
	TestEqual(TEXT("Vaporize uses elemental attack squared formula"),
	          Result.FormulaId,
	          FName(TEXT("Element.ElementAttackSquared")));
	TestEqual(TEXT("Vaporize radius comes from CSV"), Result.RadiusCm, 150.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Grass;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 9.0f);
	TestEqual(TEXT("Growth reaction id is lightning over grass"), Result.ReactionId, FName(TEXT("Y_ER_L_G")));
	TestEqual(TEXT("Growth radius comes from CSV"), Result.RadiusCm, 200.0f);
	TestEqual(TEXT("Growth does not grant elemental immunity"), State.ImmunityUntil, 0.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 12.0f, 1.05f, 10.0f);
	TestEqual(TEXT("Conduct reaction id is lightning over water"), Result.ReactionId, FName(TEXT("Y_ER_L_W")));
	TestEqual(TEXT("Conduct pure result leaves chain damage to world execution"), Result.Damage, 0.0f);
	TestEqual(
	    TEXT("Conduct uses chain element attack formula"), Result.FormulaId, FName(TEXT("Element.ChainElementAttack")));
	TestEqual(TEXT("Conduct radius comes from CSV"), Result.RadiusCm, 200.0f);
	TestTrue(TEXT("Conduct grants elemental immunity"), State.ImmunityUntil > 0.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f);
	TestEqual(TEXT("Grass over water has its own ordered reaction id"), Result.ReactionId, FName(TEXT("Y_ER_G_W")));
	TestTrue(TEXT("Enhance primes next reaction"), State.bEnhancedNextReaction);
	TestEqual(TEXT("Enhancement multiplier comes from CSV"), State.EnhancementMultiplier, 2.0f);
	TestEqual(TEXT("Grass over water blocks water reattachment"), State.BlockedAttachment, EReEchoElement::Water);
	TestEqual(TEXT("Enhancement reaction itself causes no damage"), Result.Damage, 0.0f);
	TestEqual(TEXT("Grass over water enhance does not grant immunity"), State.ImmunityUntil, 0.0f);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Water, 7.0f);
	TestTrue(TEXT("Blocked water cannot reattach after grass over water"), Result.bBlockedByImmunity);
	TestEqual(TEXT("Blocked water leaves attachment clear"), State.Attached, EReEchoElement::None);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Grass;
	State.bEnhancedNextReaction = true;
	State.EnhancementMultiplier = 2.0f;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Water, 10.0f);
	TestTrue(TEXT("Second enhancement reaction does not stack beyond table multiplier"), State.bEnhancedNextReaction);
	TestEqual(TEXT("Enhancement remains non-stacked"), State.EnhancementMultiplier, 2.0f);
	TestEqual(TEXT("Water over grass has a distinct ordered reaction id"), Result.ReactionId, FName(TEXT("Y_ER_W_G")));
	TestFalse(TEXT("Enhancement reaction does not consume prior enhancement"), Result.bAppliedEnhancement);
	TestEqual(TEXT("Water over grass blocks grass reattachment"), State.BlockedAttachment, EReEchoElement::Grass);
	TestEqual(TEXT("Water over grass enhance does not grant immunity"), State.ImmunityUntil, 0.0f);

	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 7.0f);
	TestTrue(TEXT("Blocked attachment prevents corresponding element from reattaching"), Result.bBlockedByImmunity);
	TestEqual(TEXT("Blocked element is not attached"), State.Attached, EReEchoElement::None);

	State.Attached = EReEchoElement::Grass;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Flame, 10.0f);
	TestTrue(TEXT("Primed enhancement affects next damaging reaction"), Result.bAppliedEnhancement);
	TestEqual(TEXT("Pure damaging reaction still leaves damage to execution"), Result.Damage, 0.0f);
	TestFalse(TEXT("Enhancement clears after damaging reaction"), State.bEnhancedNextReaction);
	TestEqual(TEXT("Next reaction clears attachment block"), State.BlockedAttachment, EReEchoElement::None);

	const FReEchoCsvLoadResult ChangedValueLoad =
	    FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(AssembleElementCsvFixture(TEXT("ReactionValueChanged")));
	if (!TestTrue(TEXT("Reaction value changed fixture loads"), ChangedValueLoad.bSuccess))
	{
		AddError(ChangedValueLoad.FormatIssues());
		return false;
	}
	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 10.0f);
	TestEqual(TEXT("Reaction coefficient change is visible without recompiling C++"),
	          Result.FormulaId,
	          FName(TEXT("Element.ChainElementAttack")));

	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementReactionBurnRefreshTest,
                                 "ReEcho.Combat.ElementReactionBurnRefresh",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementReactionBurnRefreshTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads for burn refresh"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	FReEchoElementWorldFixture Fixture;
	AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 60);
	Target->EditElementState().Attached = EReEchoElement::Grass;
	ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 999.0f, Fixture.MakeContext(10.0f));
	Fixture.Advance(2.1f);
	TestEqual(TEXT("Initial burn applies two elapsed GAS ticks"), EnemyHealth(Target), 980.0f);

	ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Grass, 0.0f, Fixture.MakeContext(10.0f));
	TestEqual(
	    TEXT("Grass can reattach after burn immunity expires"), Target->GetAttachedElement(), EReEchoElement::Grass);
	const FReEchoElementExecutionResult RefreshResult =
	    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 999.0f, Fixture.MakeContext(10.0f));
	TestEqual(TEXT("RefreshOnly keeps a single DOT stream"), RefreshResult.DotTicksScheduled, 3);
	TestTrue(TEXT("Refresh extends burn status duration"),
	         Target->GetElementState().ActiveStatusUntilSeconds.FindRef(FName(TEXT("Z_Burn"))) > 5.0f);

	Fixture.Advance(0.9f);
	TestEqual(TEXT("Refresh does not add an independent tick at the old third boundary"), EnemyHealth(Target), 970.0f);
	Fixture.Advance(2.0f);
	TestEqual(TEXT("Refreshed burn continues one tick per second"), EnemyHealth(Target), 950.0f);
	Fixture.Advance(0.25f);
	TestEqual(TEXT("Refreshed burn stops after its extended end window"), EnemyHealth(Target), 950.0f);

	AReEchoEnemyActor* BoundaryTarget = Fixture.SpawnEnemy(FVector(200.0f, 0.0f, 0.0f), 61);
	BoundaryTarget->EditElementState().Attached = EReEchoElement::Grass;
	ReEchoElementReaction::ApplyHitToWorld(*BoundaryTarget, EReEchoElement::Flame, 999.0f, Fixture.MakeContext(10.0f));
	Fixture.Advance(1.0f);
	TestEqual(TEXT("Boundary refresh setup applies first burn tick"), EnemyHealth(BoundaryTarget), 990.0f);
	Fixture.AdvanceTimeOnly(1.0f);
	TestTrue(TEXT("Boundary refresh setup parks on the second tick boundary"),
	         FMath::IsNearlyEqual(Fixture.World->GetTimeSeconds(),
	                              BoundaryTarget->GetElementState().BurnNextTickTimeSeconds,
	                              KINDA_SMALL_NUMBER));
	BoundaryTarget->EditElementState().Attached = EReEchoElement::Grass;
	const FReEchoElementExecutionResult BoundaryRefreshResult = ReEchoElementReaction::ApplyHitToWorld(
	    *BoundaryTarget, EReEchoElement::Flame, 999.0f, Fixture.MakeContext(10.0f));
	TestEqual(TEXT("Refresh on an exact tick boundary settles the due tick once"), EnemyHealth(BoundaryTarget), 980.0f);
	TestEqual(TEXT("Boundary refresh keeps one future DOT stream"), BoundaryRefreshResult.DotTicksScheduled, 3);
	Fixture.Advance(1.0f);
	TestEqual(TEXT("Boundary refresh applies the next stream tick once"), EnemyHealth(BoundaryTarget), 970.0f);
	Fixture.Advance(1.0f);
	TestEqual(TEXT("Boundary refresh applies one DOT tick per second"), EnemyHealth(BoundaryTarget), 960.0f);
	Fixture.Advance(1.0f);
	TestEqual(TEXT("Boundary refresh preserves the final extended boundary tick"), EnemyHealth(BoundaryTarget), 950.0f);
	Fixture.Advance(0.25f);
	TestEqual(
	    TEXT("Boundary refresh does not duplicate after the final boundary"), EnemyHealth(BoundaryTarget), 950.0f);

	{
		FReEchoElementWorldFixture StalledFixture;
		AReEchoEnemyActor* StalledTarget = StalledFixture.SpawnEnemy(FVector::ZeroVector, 62);
		StalledTarget->EditElementState().Attached = EReEchoElement::Grass;
		ReEchoElementReaction::ApplyHitToWorld(
		    *StalledTarget, EReEchoElement::Flame, 999.0f, StalledFixture.MakeContext(10.0f));
		StalledFixture.AdvanceTimeOnly(10.0f);
		TestEqual(
		    TEXT("Stalled old burn has not been actor-ticked before the new hit"), EnemyHealth(StalledTarget), 1000.0f);
		TestTrue(TEXT("Stalled old burn still carries its original end time"),
		         FMath::IsNearlyEqual(
		             StalledTarget->GetElementState().ActiveStatusUntilSeconds.FindRef(FName(TEXT("Z_Burn"))),
		             3.0f,
		             KINDA_SMALL_NUMBER));

		StalledTarget->EditElementState().Attached = EReEchoElement::Grass;
		const FReEchoElementExecutionResult StalledRefreshResult = ReEchoElementReaction::ApplyHitToWorld(
		    *StalledTarget, EReEchoElement::Flame, 999.0f, StalledFixture.MakeContext(10.0f));
		TestEqual(TEXT("Stalled refresh only settles old ticks through the old burn end"),
		          EnemyHealth(StalledTarget),
		          970.0f);
		TestEqual(TEXT("Stalled refresh starts one new DOT stream"), StalledRefreshResult.DotTicksScheduled, 3);
		TestTrue(TEXT("Stalled refresh schedules the new burn from the current frame"),
		         FMath::IsNearlyEqual(StalledTarget->GetElementState().BurnNextTickTimeSeconds, 11.0f, 0.02f));
		TestTrue(TEXT("Stalled refresh commits the new burn end after old settlement"),
		         FMath::IsNearlyEqual(
		             StalledTarget->GetElementState().ActiveStatusUntilSeconds.FindRef(FName(TEXT("Z_Burn"))),
		             13.0f,
		             0.02f));
		StalledFixture.Advance(0.99f);
		TestEqual(TEXT("Stalled refresh produces no ghost ticks between old end and new first tick"),
		          EnemyHealth(StalledTarget),
		          970.0f);
		StalledFixture.Advance(0.01f);
		TestEqual(TEXT("Stalled refresh applies the new first boundary tick once"), EnemyHealth(StalledTarget), 960.0f);
		StalledFixture.Advance(1.0f);
		TestEqual(
		    TEXT("Stalled refresh applies the new second boundary tick once"), EnemyHealth(StalledTarget), 950.0f);
		StalledFixture.Advance(1.0f);
		TestEqual(TEXT("Stalled refresh preserves the new final boundary tick"), EnemyHealth(StalledTarget), 940.0f);
		StalledFixture.Advance(0.25f);
		TestEqual(TEXT("Stalled refresh does not duplicate after the new final boundary"),
		          EnemyHealth(StalledTarget),
		          940.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementReactionSaveContinuityTest,
                                 "ReEcho.Combat.ElementReactionSaveContinuity",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementReactionSaveContinuityTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads for burn save continuity"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	FReEchoEnemyRuntimeState SavedState;
	{
		FReEchoElementWorldFixture SourceFixture;
		AReEchoEnemyActor* SourceTarget = SourceFixture.SpawnEnemy(FVector::ZeroVector, 70, 28.0f);
		SourceTarget->EditElementState().Attached = EReEchoElement::Grass;
		ReEchoElementReaction::ApplyHitToWorld(
		    *SourceTarget, EReEchoElement::Flame, 999.0f, SourceFixture.MakeContext(5.0f));
		SourceFixture.Advance(1.2f);
		TestEqual(TEXT("Source burn tick applied before capture"), EnemyHealth(SourceTarget), 23.0f);

		SavedState = SourceTarget->CaptureRuntimeState();
		TestTrue(TEXT("Active burn status saves as remaining duration"),
		         FMath::IsNearlyEqual(
		             SavedState.ElementState.ActiveStatusUntilSeconds.FindRef(FName(TEXT("Z_Burn"))), 1.8f, 0.02f));
		TestTrue(TEXT("Element immunity saves as remaining duration"),
		         FMath::IsNearlyEqual(SavedState.ElementState.ImmunityUntil, 0.8f, 0.02f));
		TestTrue(TEXT("Next burn tick saves as remaining duration"),
		         FMath::IsNearlyEqual(SavedState.ElementState.BurnNextTickTimeSeconds, 0.8f, 0.02f));
	}

	{
		FReEchoElementWorldFixture RestoreFixture;
		RestoreFixture.Advance(5.0f);
		AReEchoEnemyActor* RestoredTarget = RestoreFixture.SpawnEnemy(FVector::ZeroVector, 71);
		RestoredTarget->RestoreRuntimeState(SavedState);
		TestTrue(TEXT("Burn status restore rebases onto new world time"),
		         FMath::IsNearlyEqual(
		             RestoredTarget->GetElementState().ActiveStatusUntilSeconds.FindRef(FName(TEXT("Z_Burn"))),
		             6.8f,
		             0.02f));
		TestTrue(TEXT("Burn next tick restore rebases onto new world time"),
		         FMath::IsNearlyEqual(RestoredTarget->GetElementState().BurnNextTickTimeSeconds, 5.8f, 0.02f));
		RestoreFixture.Advance(0.79f);
		TestEqual(TEXT("Restored burn waits for remaining next tick"), EnemyHealth(RestoredTarget), 23.0f);
		RestoreFixture.Advance(0.01f);
		TestEqual(
		    TEXT("Restored burn resumes GAS damage on next deterministic tick"), EnemyHealth(RestoredTarget), 18.0f);
		RestoreFixture.Advance(1.0f);
		TestEqual(TEXT("Restored burn applies final deterministic boundary tick"), EnemyHealth(RestoredTarget), 13.0f);
		RestoreFixture.Advance(0.25f);
		TestEqual(TEXT("Restored burn does not tick after restored end"), EnemyHealth(RestoredTarget), 13.0f);
	}

	FReEchoEnemyRuntimeState DueNowSavedState;
	{
		FReEchoElementWorldFixture SourceFixture;
		AReEchoEnemyActor* SourceTarget = SourceFixture.SpawnEnemy(FVector::ZeroVector, 72, 28.0f);
		SourceTarget->EditElementState().Attached = EReEchoElement::Grass;
		ReEchoElementReaction::ApplyHitToWorld(
		    *SourceTarget, EReEchoElement::Flame, 999.0f, SourceFixture.MakeContext(5.0f));
		SourceFixture.Advance(1.0f);
		TestEqual(TEXT("Due-now save setup applies first tick"), EnemyHealth(SourceTarget), 23.0f);
		SourceFixture.AdvanceTimeOnly(1.0f);
		DueNowSavedState = SourceTarget->CaptureRuntimeState();
		TestTrue(TEXT("Due-now burn remains active in save state"), DueNowSavedState.ElementState.bBurnActive);
		TestTrue(TEXT("Due-now burn next tick saves as zero remaining duration"),
		         FMath::IsNearlyEqual(DueNowSavedState.ElementState.BurnNextTickTimeSeconds, 0.0f, KINDA_SMALL_NUMBER));
		TestTrue(
		    TEXT("Due-now burn status saves remaining duration"),
		    FMath::IsNearlyEqual(
		        DueNowSavedState.ElementState.ActiveStatusUntilSeconds.FindRef(FName(TEXT("Z_Burn"))), 1.0f, 0.02f));
	}

	{
		FReEchoElementWorldFixture RestoreFixture;
		RestoreFixture.Advance(5.0f);
		AReEchoEnemyActor* RestoredTarget = RestoreFixture.SpawnEnemy(FVector::ZeroVector, 73);
		RestoredTarget->RestoreRuntimeState(DueNowSavedState);
		TestTrue(TEXT("Due-now restore keeps burn active"), RestoredTarget->GetElementState().bBurnActive);
		TestTrue(TEXT("Due-now restore rebases next tick onto the current world time"),
		         FMath::IsNearlyEqual(RestoredTarget->GetElementState().BurnNextTickTimeSeconds,
		                              RestoreFixture.World->GetTimeSeconds(),
		                              KINDA_SMALL_NUMBER));
		RestoreFixture.Advance(0.0f);
		TestEqual(
		    TEXT("Due-now restored burn applies the immediate pending tick once"), EnemyHealth(RestoredTarget), 18.0f);
		RestoreFixture.Advance(1.0f);
		TestEqual(TEXT("Due-now restored burn preserves the final boundary tick"), EnemyHealth(RestoredTarget), 13.0f);
		RestoreFixture.Advance(0.25f);
		TestEqual(
		    TEXT("Due-now restored burn does not duplicate after final boundary"), EnemyHealth(RestoredTarget), 13.0f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoElementReactionWorldTest,
                                 "ReEcho.Combat.ElementReactionWorld",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoElementReactionWorldTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads for world element reactions"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 1);
		UReEchoCombatEventsComponent* Events = Target->FindComponentByClass<UReEchoCombatEventsComponent>();
		TestNotNull(TEXT("Element target owns Combat events"), Events);
		FReEchoElementHitContext Context = Fixture.MakeContext(10.0f);
		Context.Attack.Sequence = 7301;
		ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Grass, 0.0f, Context);
		TestEqual(TEXT("Ordinary attachment publishes no resolved-reaction event"),
		          Events ? Events->GetElementReactionPublishCountForTests() : -1,
		          0);
		const FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 999.0f, Context);
		TestEqual(TEXT("Valid reaction publishes exactly once after settlement"),
		          Events ? Events->GetElementReactionPublishCountForTests() : -1,
		          1);
		if (Events)
		{
			const FReEchoElementReactionResolvedEvent& Event = Events->GetLastElementReactionEventForTests();
			TestEqual(TEXT("Resolved event keeps attack identity"), Event.Attack.Sequence, int64(7301));
			TestEqual(TEXT("Resolved event keeps reaction id"), Event.ReactionId, FName(TEXT("Y_ER_F_G")));
			TestEqual(TEXT("Resolved event keeps behavior id"),
			          Event.ReactionBehaviorId,
			          FName(TEXT("Reaction.Burn")));
			TestEqual(TEXT("Resolved event keeps settled radius"), Event.RadiusCm, 0.0f);
			TestEqual(TEXT("Resolved event keeps previous attachment"), Event.PreviousElement, EReEchoElement::Grass);
			TestEqual(TEXT("Resolved event keeps incoming element"), Event.IncomingElement, EReEchoElement::Flame);
			TestEqual(TEXT("Resolved event keeps resulting attachment"), Event.ResultingElement, EReEchoElement::None);
			TestEqual(TEXT("Resolved event contains the authoritative primary target once"), Event.AffectedTargets.Num(), 1);
			TestTrue(TEXT("Resolved event primary target matches gameplay"), Event.AffectedTargets[0].Get() == Target);
		}
		TestEqual(TEXT("Burn has no immediate damage"), Result.ImmediateDamageApplied, 0.0f);
		TestEqual(TEXT("Burn keeps one deterministic tick per second for three seconds"), Result.DotTicksScheduled, 3);
		TestEqual(TEXT("Burn records three DOT delays"), Result.DotTickDelaySeconds.Num(), 3);
		TestEqual(TEXT("Burn first DOT delay is one second"), Result.DotTickDelaySeconds[0], 1.0f);
		TestEqual(TEXT("Burn second DOT delay is two seconds"), Result.DotTickDelaySeconds[1], 2.0f);
		TestEqual(TEXT("Burn third DOT delay is three seconds"), Result.DotTickDelaySeconds[2], 3.0f);
		TestEqual(TEXT("Burn does not treat base damage as DOT"), EnemyHealth(Target), 1000.0f);
		Fixture.Advance(0.99f);
		TestEqual(TEXT("Burn does not tick before first one-second boundary"), EnemyHealth(Target), 1000.0f);
		Fixture.Advance(0.01f);
		TestEqual(TEXT("Burn first tick applies GAS damage"), EnemyHealth(Target), 990.0f);
		Fixture.Advance(1.0f);
		TestEqual(TEXT("Burn second tick applies GAS damage"), EnemyHealth(Target), 980.0f);
		Fixture.Advance(1.0f);
		TestEqual(TEXT("Burn final boundary tick applies at status end"), EnemyHealth(Target), 970.0f);
		Fixture.Advance(0.25f);
		TestEqual(TEXT("Burn does not tick after status end"), EnemyHealth(Target), 970.0f);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AActor* BurnSource = Fixture.SpawnSource(10.0f, 1.0f);
		AReEchoEnemyActor* ShieldTarget = Fixture.SpawnEnemy(FVector::ZeroVector, 3, 1000.0f, EReEchoEnemyKind::Shield);
		ShieldTarget->EditElementState().Attached = EReEchoElement::Grass;
		FReEchoElementHitContext Context = Fixture.MakeContext(10.0f);
		Context.Attack = Fixture.MakeAttack(BurnSource);
		Context.SourceLocation = FVector(-100.0f, 0.0f, 0.0f);
		ReEchoElementReaction::ApplyHitToWorld(*ShieldTarget, EReEchoElement::Flame, 999.0f, Context);
		TestEqual(TEXT("Burn preserves the in-run source actor"),
		          ShieldTarget->GetElementState().BurnAttack.Source.Get(),
		          BurnSource);
		TestTrue(TEXT("Burn preserves the original source location"),
		         FVector::DistSquared(ShieldTarget->GetElementState().BurnSourceLocation, Context.SourceLocation) <=
		             KINDA_SMALL_NUMBER);

		const FReEchoEnemyRuntimeState SavedShieldState = ShieldTarget->CaptureRuntimeState();
		TestFalse(TEXT("Burn save state records attack source fallback as unavailable"),
		          SavedShieldState.ElementState.BurnAttack.IsValid());
		TestTrue(TEXT("Burn save state persists deterministic SourceLocation"),
		         FVector::DistSquared(SavedShieldState.ElementState.BurnSourceLocation, Context.SourceLocation) <=
		             KINDA_SMALL_NUMBER);

		Fixture.Advance(1.0f);
		TestEqual(TEXT("Shield burn DOT uses the original behind-source location through GAS"),
		          EnemyHealth(ShieldTarget),
		          980.0f);

		FReEchoElementWorldFixture RestoreFixture;
		RestoreFixture.Advance(5.0f);
		AReEchoEnemyActor* RestoredShield = RestoreFixture.SpawnEnemy(FVector::ZeroVector, 4);
		RestoredShield->RestoreRuntimeState(SavedShieldState);
		TestFalse(TEXT("Restored burn attack source falls back to null"),
		          RestoredShield->GetElementState().BurnAttack.IsValid());
		TestTrue(TEXT("Restored burn keeps deterministic SourceLocation for shield direction"),
		         FVector::DistSquared(RestoredShield->GetElementState().BurnSourceLocation, Context.SourceLocation) <=
		             KINDA_SMALL_NUMBER);
		RestoreFixture.Advance(1.0f);
		TestEqual(TEXT("Restored shield burn DOT still passes shield direction and GAS damage"),
		          EnemyHealth(RestoredShield),
		          35.0f);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 2);
		Target->EditElementState().Attached = EReEchoElement::Water;
		const FReEchoElementExecutionResult Result = ReEchoElementReaction::ApplyHitToWorld(
		    *Target, EReEchoElement::Flame, 999.0f, Fixture.MakeContext(10.0f, 0.25f));
		TestEqual(TEXT("Vaporize uses elemental attack squared damage"), Result.ImmediateDamageApplied, 25.0f);
		TestEqual(TEXT("Vaporize damage is applied through GAS health"), EnemyHealth(Target), 975.0f);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Primary = Fixture.SpawnEnemy(FVector(0.0f, 0.0f, 0.0f), 10);
		AReEchoEnemyActor* Left = Fixture.SpawnEnemy(FVector(-50.0f, 0.0f, 0.0f), 11);
		AReEchoEnemyActor* Right = Fixture.SpawnEnemy(FVector(50.0f, 0.0f, 0.0f), 12);
		AReEchoEnemyActor* Far = Fixture.SpawnEnemy(FVector(0.0f, 150.0f, 0.0f), 13);
		AReEchoEnemyActor* Immune = Fixture.SpawnEnemy(FVector(0.0f, 90.0f, 0.0f), 15);
		AReEchoEnemyActor* Blocked = Fixture.SpawnEnemy(FVector(0.0f, -90.0f, 0.0f), 16);
		AReEchoEnemyActor* Outside = Fixture.SpawnEnemy(FVector(250.0f, 0.0f, 0.0f), 14);
		Primary->EditElementState().Attached = EReEchoElement::Grass;
		Immune->EditElementState().ImmunityUntil = 30.0f;
		Blocked->EditElementState().BlockedAttachment = EReEchoElement::Grass;

		const FReEchoElementExecutionResult Result = ReEchoElementReaction::ApplyHitToWorld(
		    *Primary, EReEchoElement::Lightning, 0.0f, Fixture.MakeContext(10.0f, 1.0f, 0.5f));
		TestEqual(TEXT("Growth scales radius by ReactionEfficiency and affects three allowed targets"),
		          Result.AffectedTargets.Num(),
		          3);
		TestEqual(TEXT("Growth stable order starts from primary"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[0])),
		          Primary);
		TestEqual(TEXT("Growth stable tie order uses location"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[1])),
		          Left);
		TestEqual(TEXT("Growth stable tie order keeps right target after left"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[2])),
		          Right);
		TestEqual(TEXT("Growth attaches grass to primary"), Primary->GetAttachedElement(), EReEchoElement::Grass);
		TestEqual(TEXT("Growth attaches grass to left target"), Left->GetAttachedElement(), EReEchoElement::Grass);
		TestEqual(TEXT("Growth attaches grass to right target"), Right->GetAttachedElement(), EReEchoElement::Grass);
		TestEqual(TEXT("Growth scaled radius excludes far target"), Far->GetAttachedElement(), EReEchoElement::None);
		TestEqual(TEXT("Growth respects elemental immunity"), Immune->GetAttachedElement(), EReEchoElement::None);
		TestEqual(TEXT("Growth respects blocked attachment"), Blocked->GetAttachedElement(), EReEchoElement::None);
		TestEqual(TEXT("Growth ignores outside target"), Outside->GetAttachedElement(), EReEchoElement::None);
		TestEqual(
		    TEXT("Growth does not grant immunity to attached target"), Primary->GetElementState().ImmunityUntil, 0.0f);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Primary = Fixture.SpawnEnemy(FVector::ZeroVector, 20);
		AReEchoEnemyActor* Left = Fixture.SpawnEnemy(FVector(-80.0f, 0.0f, 0.0f), 21);
		AReEchoEnemyActor* Up = Fixture.SpawnEnemy(FVector(0.0f, 80.0f, 0.0f), 22);
		AReEchoEnemyActor* Right = Fixture.SpawnEnemy(FVector(80.0f, 0.0f, 0.0f), 23);
		AReEchoEnemyActor* Bridge = Fixture.SpawnEnemy(FVector(160.0f, 0.0f, 0.0f), 24);
		AReEchoEnemyActor* Outside = Fixture.SpawnEnemy(FVector(400.0f, 0.0f, 0.0f), 25);
		for (AReEchoEnemyActor* Enemy : {Primary, Left, Up, Right, Bridge, Outside})
		{
			Enemy->EditElementState().Attached = EReEchoElement::Water;
		}

		const FReEchoElementExecutionResult Result = ReEchoElementReaction::ApplyHitToWorld(
		    *Primary, EReEchoElement::Lightning, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Conduct chain deduplicates visited targets"), Result.AffectedTargets.Num(), 5);
		UReEchoCombatEventsComponent* Events = Primary->FindComponentByClass<UReEchoCombatEventsComponent>();
		TestNotNull(TEXT("Conduct primary owns Combat events"), Events);
		if (Events)
		{
			const FReEchoElementReactionResolvedEvent& Event = Events->GetLastElementReactionEventForTests();
			TestEqual(TEXT("Conduct publishes exactly one resolved event"),
			          Events->GetElementReactionPublishCountForTests(),
			          1);
			TestEqual(TEXT("Conduct event retains configured radius"), Event.RadiusCm, 200.0f);
			TestEqual(TEXT("Conduct event retains authoritative target count"), Event.AffectedTargets.Num(), 5);
			TestEqual(TEXT("Conduct event retains one authoritative edge per discovered secondary target"),
			          Event.ReactionLinks.Num(),
			          4);
			if (Event.ReactionLinks.Num() == 4)
			{
				TestTrue(TEXT("Conduct first edge starts at the primary"),
				         Event.ReactionLinks[0].SourceTarget.Get() == Primary);
				TestTrue(TEXT("Conduct first edge reaches the stable first neighbor"),
				         Event.ReactionLinks[0].TargetTarget.Get() == Left);
				TestTrue(TEXT("Conduct bridge edge keeps its gameplay discovery parent"),
				         Event.ReactionLinks[3].SourceTarget &&
				             Event.AffectedTargets.Contains(Event.ReactionLinks[3].SourceTarget) &&
				             Event.ReactionLinks[3].TargetTarget.Get() == Bridge);
			}
			for (int32 Index = 0; Index < Result.AffectedTargets.Num(); ++Index)
			{
				TestTrue(TEXT("Conduct event target order matches gameplay execution"),
				         Event.AffectedTargets[Index].Get() == Result.AffectedTargets[Index].Get());
			}
		}
		TestEqual(TEXT("Conduct starts with primary target"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[0])),
		          Primary);
		TestEqual(TEXT("Conduct visits stable left neighbor first"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[1])),
		          Left);
		TestEqual(TEXT("Conduct visits stable up neighbor second"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[2])),
		          Up);
		TestEqual(TEXT("Conduct visits stable right neighbor third"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[3])),
		          Right);
		TestEqual(TEXT("Conduct chains to one-meter bridge target"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[4])),
		          Bridge);
		TestEqual(TEXT("Conduct applies workbook formula GAS damage to all chained targets"),
		          Result.ImmediateDamageApplied,
		          120.0f);
		TestEqual(TEXT("Conduct primary health reduced"), EnemyHealth(Primary), 976.0f);
		TestEqual(TEXT("Conduct left health reduced"), EnemyHealth(Left), 976.0f);
		TestEqual(TEXT("Conduct up health reduced"), EnemyHealth(Up), 976.0f);
		TestEqual(TEXT("Conduct right health reduced"), EnemyHealth(Right), 976.0f);
		TestEqual(TEXT("Conduct bridge health reduced"), EnemyHealth(Bridge), 976.0f);
		TestEqual(TEXT("Conduct outside target untouched"), EnemyHealth(Outside), 1000.0f);
		TestEqual(TEXT("Conduct clears primary attachment"), Primary->GetAttachedElement(), EReEchoElement::None);
	}

	{
		const FReEchoCsvLoadResult SmallRadiusLoad = FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(
		    AssembleElementCsvFixture(TEXT("ReactionValueChanged")));
		if (!TestTrue(TEXT("Small conduct radius fixture loads"), SmallRadiusLoad.bSuccess))
		{
			AddError(SmallRadiusLoad.FormatIssues());
			return false;
		}

		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Primary = Fixture.SpawnEnemy(FVector::ZeroVector, 26);
		AReEchoEnemyActor* Near = Fixture.SpawnEnemy(FVector(74.0f, 0.0f, 0.0f), 27);
		AReEchoEnemyActor* HiddenFloorTarget = Fixture.SpawnEnemy(FVector(0.0f, 90.0f, 0.0f), 28);
		for (AReEchoEnemyActor* Enemy : {Primary, Near, HiddenFloorTarget})
		{
			Enemy->EditElementState().Attached = EReEchoElement::Water;
		}

		const FReEchoElementExecutionResult Result = ReEchoElementReaction::ApplyHitToWorld(
		    *Primary, EReEchoElement::Lightning, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Conduct uses configured 75 cm radius exactly"), Result.AffectedTargets.Num(), 2);
		TestEqual(TEXT("Conduct small radius includes near target"),
		          Cast<AReEchoEnemyActor>(ResolveWeak(Result.AffectedTargets[1])),
		          Near);
		TestEqual(
		    TEXT("Conduct small radius excludes former hidden-floor target"), EnemyHealth(HiddenFloorTarget), 1000.0f);
		TestEqual(
		    TEXT("Conduct fixture uses DamageIncrease in workbook formula"), Result.ImmediateDamageApplied, 72.0f);
		FReEchoCsvDataRegistry::LoadAndPublishDefault();
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 30);
		Target->EditElementState().Attached = EReEchoElement::Grass;
		FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Water, 50.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Enhancement reaction does not damage"), Result.ImmediateDamageApplied, 0.0f);
		TestTrue(TEXT("Enhancement is primed"), Target->GetElementState().bEnhancedNextReaction);
		TestEqual(TEXT("Water over grass blocks grass reattachment"),
		          Target->GetElementState().BlockedAttachment,
		          EReEchoElement::Grass);
		TestEqual(TEXT("Enhancement does not grant elemental immunity"), Target->GetElementState().ImmunityUntil, 0.0f);

		Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Grass, 7.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Blocked element still follows normal damage path"), Result.ImmediateDamageApplied, 7.0f);
		TestEqual(TEXT("Blocked element does not reattach"), Target->GetAttachedElement(), EReEchoElement::None);

		Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Water, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Allowed attachment can be applied while enhance grants no immunity"),
		          Target->GetAttachedElement(),
		          EReEchoElement::Water);
		Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Next damaging reaction gains +100 percent final damage"), Result.ImmediateDamageApplied, 50.0f);
		TestFalse(TEXT("Next reaction clears primed enhancement"), Target->GetElementState().bEnhancedNextReaction);
		TestEqual(TEXT("Next reaction clears blocked attachment"),
		          Target->GetElementState().BlockedAttachment,
		          EReEchoElement::None);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AActor* PlayerSource = Fixture.SpawnSource(10.0f, 0.25f);
		AActor* EchoSource = Fixture.SpawnSource(10.0f, 2.0f);
		AReEchoEnemyActor* PlayerTarget = Fixture.SpawnEnemy(FVector::ZeroVector, 40);
		AReEchoEnemyActor* EchoTarget = Fixture.SpawnEnemy(FVector(300.0f, 0.0f, 0.0f), 41);
		PlayerTarget->EditElementState().Attached = EReEchoElement::Water;
		EchoTarget->EditElementState().Attached = EReEchoElement::Water;
		const float PlayerApplied = PlayerTarget->ReceiveElementalDamage(
		    999.0f, EReEchoElement::Flame, FVector::ZeroVector, 1.0f, Fixture.MakeAttack(PlayerSource));
		const float EchoApplied = EchoTarget->ReceiveElementalDamage(
		    999.0f, EReEchoElement::Flame, FVector::ZeroVector, 1.0f, Fixture.MakeAttack(EchoSource));
		TestEqual(TEXT("Player and echo route through the same elemental damage path"), PlayerApplied, EchoApplied);
		TestEqual(
		    TEXT("AffectedByEchoEfficiency=false keeps current workbook reactions consistent"), PlayerApplied, 25.0f);
	}

	{
		const FReEchoCsvLoadResult EchoEfficiencyLoad = FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(
		    AssembleElementCsvFixture(TEXT("EchoEfficiencyEnabled")));
		if (!TestTrue(TEXT("Echo efficiency fixture loads"), EchoEfficiencyLoad.bSuccess))
		{
			AddError(EchoEfficiencyLoad.FormatIssues());
			return false;
		}
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 50);
		Target->EditElementState().Attached = EReEchoElement::Water;
		const FReEchoElementExecutionResult Result = ReEchoElementReaction::ApplyHitToWorld(
		    *Target, EReEchoElement::Flame, 0.0f, Fixture.MakeContext(10.0f, 2.0f));
		TestEqual(
		    TEXT("AffectedByEchoEfficiency=true multiplies reaction damage"), Result.ImmediateDamageApplied, 50.0f);
	}

	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	return true;
}

#endif
