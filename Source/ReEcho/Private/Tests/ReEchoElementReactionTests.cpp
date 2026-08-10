#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ReEchoElementReaction.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

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

	AReEchoEnemyActor* SpawnEnemy(const FVector& Location, const int32 SpawnIndex)
	{
		FActorSpawnParameters SpawnParams;
		AReEchoEnemyActor* Enemy = World->SpawnActor<AReEchoEnemyActor>(Location, FRotator::ZeroRotator, SpawnParams);
		Enemy->Configure(EReEchoEnemyKind::Grunt, SpawnIndex);
		FReEchoStatBlock Stats;
		Stats.HpMax = 1000.0f;
		Stats.HpPoint = 1000.0f;
		Stats.Block = 0;
		Enemy->GetCombatantComponent()->BindToAbilitySystem(Enemy->GetAbilitySystemComponent());
		Enemy->GetCombatantComponent()->InitializeFromStats(Stats, true);
		return Enemy;
	}

	AActor* SpawnSource(const float ElementalAttack, const float EchoEfficiency)
	{
		AActor* Source = World->SpawnActor<AActor>();
		UReEchoCombatantComponent* SourceCombatant = NewObject<UReEchoCombatantComponent>(Source, TEXT("SourceCombatant"));
		Source->AddInstanceComponent(SourceCombatant);
		SourceCombatant->RegisterComponent();
		FReEchoStatBlock Stats;
		Stats.ElementalAttack = ElementalAttack;
		Stats.EchoEfficiency = EchoEfficiency;
		SourceCombatant->InitializeFromStats(Stats, true);
		return Source;
	}

	FReEchoElementHitContext MakeContext(const float ElementalAttack,
	                                    const float EchoEfficiency = 1.0f,
	                                    const float ReactionEfficiency = 1.0f)
	{
		FReEchoElementHitContext Context;
		Context.SourceElementalAttack = ElementalAttack;
		Context.SourceEchoEfficiency = EchoEfficiency;
		Context.ReactionEfficiency = ReactionEfficiency;
		Context.SourceLocation = FVector::ZeroVector;
		return Context;
	}

	void Advance(const float Seconds)
	{
		World->Tick(ELevelTick::LEVELTICK_All, Seconds);
		World->GetTimerManager().Tick(Seconds);
	}
};

float EnemyHealth(const AReEchoEnemyActor* Enemy)
{
	return Enemy->GetCombatantComponent()->CurrentHealth;
}

template <typename ElementType>
ElementType* ResolveWeak(const TWeakObjectPtr<ElementType>& WeakObject)
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
	TestEqual(TEXT("Vaporize uses elemental attack squared formula"), Result.FormulaId, FName(TEXT("Element.ElementAttackSquared")));
	TestEqual(TEXT("Vaporize radius comes from CSV"), Result.RadiusCm, 150.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Grass;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 9.0f);
	TestEqual(TEXT("Growth reaction id is lightning over grass"), Result.ReactionId, FName(TEXT("Y_ER_L_G")));
	TestEqual(TEXT("Growth radius comes from CSV"), Result.RadiusCm, 200.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Lightning, 12.0f, 1.05f);
	TestEqual(TEXT("Conduct reaction id is lightning over water"), Result.ReactionId, FName(TEXT("Y_ER_L_W")));
	TestEqual(TEXT("Conduct pure result leaves chain damage to world execution"), Result.Damage, 0.0f);
	TestEqual(TEXT("Conduct uses chain element attack formula"), Result.FormulaId, FName(TEXT("Element.ChainElementAttack")));
	TestEqual(TEXT("Conduct radius comes from CSV"), Result.RadiusCm, 100.0f);

	State = FReEchoElementState{};
	State.Attached = EReEchoElement::Water;
	Result = ReEchoElementReaction::ResolveHit(State, EReEchoElement::Grass, 10.0f);
	TestEqual(TEXT("Grass over water has its own ordered reaction id"), Result.ReactionId, FName(TEXT("Y_ER_G_W")));
	TestTrue(TEXT("Enhance primes next reaction"), State.bEnhancedNextReaction);
	TestEqual(TEXT("Enhancement multiplier comes from CSV"), State.EnhancementMultiplier, 2.0f);
	TestEqual(TEXT("Grass over water blocks water reattachment"), State.BlockedAttachment, EReEchoElement::Water);
	TestEqual(TEXT("Enhancement reaction itself causes no damage"), Result.Damage, 0.0f);

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
	TestEqual(TEXT("Reaction coefficient change is visible without recompiling C++"), Result.FormulaId, FName(TEXT("Element.ChainElementAttack")));

	FReEchoCsvDataRegistry::LoadAndPublishDefault();
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
		Target->EditElementState().Attached = EReEchoElement::Grass;
		const FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 999.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Burn has no immediate damage"), Result.ImmediateDamageApplied, 0.0f);
		TestEqual(TEXT("Burn schedules one deterministic tick per second for three seconds"), Result.DotTicksScheduled, 3);
		TestEqual(TEXT("Burn records three DOT delays"), Result.DotTickDelaySeconds.Num(), 3);
		TestEqual(TEXT("Burn first DOT delay is one second"), Result.DotTickDelaySeconds[0], 1.0f);
		TestEqual(TEXT("Burn second DOT delay is two seconds"), Result.DotTickDelaySeconds[1], 2.0f);
		TestEqual(TEXT("Burn third DOT delay is three seconds"), Result.DotTickDelaySeconds[2], 3.0f);
		TestEqual(TEXT("Burn does not treat base damage as DOT"), EnemyHealth(Target), 1000.0f);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 2);
		Target->EditElementState().Attached = EReEchoElement::Water;
		const FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 999.0f, Fixture.MakeContext(10.0f, 0.25f));
		TestEqual(TEXT("Vaporize uses elemental attack squared damage"), Result.ImmediateDamageApplied, 25.0f);
		TestEqual(TEXT("Vaporize damage is applied through GAS health"), EnemyHealth(Target), 975.0f);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Primary = Fixture.SpawnEnemy(FVector(0.0f, 0.0f, 0.0f), 10);
		AReEchoEnemyActor* Left = Fixture.SpawnEnemy(FVector(-50.0f, 0.0f, 0.0f), 11);
		AReEchoEnemyActor* Right = Fixture.SpawnEnemy(FVector(50.0f, 0.0f, 0.0f), 12);
		AReEchoEnemyActor* Far = Fixture.SpawnEnemy(FVector(0.0f, 150.0f, 0.0f), 13);
		AReEchoEnemyActor* Outside = Fixture.SpawnEnemy(FVector(250.0f, 0.0f, 0.0f), 14);
		Primary->EditElementState().Attached = EReEchoElement::Grass;

		const FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Primary, EReEchoElement::Lightning, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Growth affects four in-radius targets"), Result.AffectedTargets.Num(), 4);
		TestEqual(TEXT("Growth stable order starts from primary"), ResolveWeak(Result.AffectedTargets[0]), Primary);
		TestEqual(TEXT("Growth stable tie order uses location"), ResolveWeak(Result.AffectedTargets[1]), Left);
		TestEqual(TEXT("Growth stable tie order keeps right target after left"), ResolveWeak(Result.AffectedTargets[2]), Right);
		TestEqual(TEXT("Growth stable distance order includes farther in-radius target"), ResolveWeak(Result.AffectedTargets[3]), Far);
		TestEqual(TEXT("Growth attaches grass to primary"), Primary->GetAttachedElement(), EReEchoElement::Grass);
		TestEqual(TEXT("Growth attaches grass to left target"), Left->GetAttachedElement(), EReEchoElement::Grass);
		TestEqual(TEXT("Growth attaches grass to right target"), Right->GetAttachedElement(), EReEchoElement::Grass);
		TestEqual(TEXT("Growth attaches grass to far target"), Far->GetAttachedElement(), EReEchoElement::Grass);
		TestEqual(TEXT("Growth ignores outside target"), Outside->GetAttachedElement(), EReEchoElement::None);
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

		const FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Primary, EReEchoElement::Lightning, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Conduct chain deduplicates visited targets"), Result.AffectedTargets.Num(), 5);
		TestEqual(TEXT("Conduct starts with primary target"), ResolveWeak(Result.AffectedTargets[0]), Primary);
		TestEqual(TEXT("Conduct visits stable left neighbor first"), ResolveWeak(Result.AffectedTargets[1]), Left);
		TestEqual(TEXT("Conduct visits stable up neighbor second"), ResolveWeak(Result.AffectedTargets[2]), Up);
		TestEqual(TEXT("Conduct visits stable right neighbor third"), ResolveWeak(Result.AffectedTargets[3]), Right);
		TestEqual(TEXT("Conduct chains to one-meter bridge target"), ResolveWeak(Result.AffectedTargets[4]), Bridge);
		TestEqual(TEXT("Conduct applies GAS damage to all chained targets"), Result.ImmediateDamageApplied, 100.0f);
		TestEqual(TEXT("Conduct primary health reduced"), EnemyHealth(Primary), 980.0f);
		TestEqual(TEXT("Conduct left health reduced"), EnemyHealth(Left), 980.0f);
		TestEqual(TEXT("Conduct up health reduced"), EnemyHealth(Up), 980.0f);
		TestEqual(TEXT("Conduct right health reduced"), EnemyHealth(Right), 980.0f);
		TestEqual(TEXT("Conduct bridge health reduced"), EnemyHealth(Bridge), 980.0f);
		TestEqual(TEXT("Conduct outside target untouched"), EnemyHealth(Outside), 1000.0f);
		TestEqual(TEXT("Conduct clears primary attachment"), Primary->GetAttachedElement(), EReEchoElement::None);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 30);
		Target->EditElementState().Attached = EReEchoElement::Grass;
		FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Water, 50.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Enhancement reaction does not damage"), Result.ImmediateDamageApplied, 0.0f);
		TestTrue(TEXT("Enhancement is primed"), Target->GetElementState().bEnhancedNextReaction);
		TestEqual(TEXT("Water over grass blocks grass reattachment"), Target->GetElementState().BlockedAttachment, EReEchoElement::Grass);

		Result = ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Grass, 7.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Blocked element still follows normal damage path"), Result.ImmediateDamageApplied, 7.0f);
		TestEqual(TEXT("Blocked element does not reattach"), Target->GetAttachedElement(), EReEchoElement::None);

		Fixture.Advance(2.1f);
		Target->EditElementState().ImmunityUntil = 0.0f;
		Result = ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Water, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Allowed attachment can be applied after immunity window"), Target->GetAttachedElement(), EReEchoElement::Water);
		Result = ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 0.0f, Fixture.MakeContext(10.0f));
		TestEqual(TEXT("Next damaging reaction gains +100 percent final damage"), Result.ImmediateDamageApplied, 50.0f);
		TestFalse(TEXT("Next reaction clears primed enhancement"), Target->GetElementState().bEnhancedNextReaction);
		TestEqual(TEXT("Next reaction clears blocked attachment"), Target->GetElementState().BlockedAttachment, EReEchoElement::None);
	}

	{
		FReEchoElementWorldFixture Fixture;
		AActor* PlayerSource = Fixture.SpawnSource(10.0f, 0.25f);
		AActor* EchoSource = Fixture.SpawnSource(10.0f, 2.0f);
		AReEchoEnemyActor* PlayerTarget = Fixture.SpawnEnemy(FVector::ZeroVector, 40);
		AReEchoEnemyActor* EchoTarget = Fixture.SpawnEnemy(FVector(300.0f, 0.0f, 0.0f), 41);
		PlayerTarget->EditElementState().Attached = EReEchoElement::Water;
		EchoTarget->EditElementState().Attached = EReEchoElement::Water;
		const float PlayerApplied =
		    PlayerTarget->ReceiveElementalDamage(999.0f, EReEchoElement::Flame, FVector::ZeroVector, PlayerSource, 1.0f);
		const float EchoApplied =
		    EchoTarget->ReceiveElementalDamage(999.0f, EReEchoElement::Flame, FVector::ZeroVector, EchoSource, 1.0f);
		TestEqual(TEXT("Player and echo route through the same elemental damage path"), PlayerApplied, EchoApplied);
		TestEqual(TEXT("AffectedByEchoEfficiency=false keeps current workbook reactions consistent"), PlayerApplied, 25.0f);
	}

	{
		const FReEchoCsvLoadResult EchoEfficiencyLoad =
		    FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(AssembleElementCsvFixture(TEXT("EchoEfficiencyEnabled")));
		if (!TestTrue(TEXT("Echo efficiency fixture loads"), EchoEfficiencyLoad.bSuccess))
		{
			AddError(EchoEfficiencyLoad.FormatIssues());
			return false;
		}
		FReEchoElementWorldFixture Fixture;
		AReEchoEnemyActor* Target = Fixture.SpawnEnemy(FVector::ZeroVector, 50);
		Target->EditElementState().Attached = EReEchoElement::Water;
		const FReEchoElementExecutionResult Result =
		    ReEchoElementReaction::ApplyHitToWorld(*Target, EReEchoElement::Flame, 0.0f, Fixture.MakeContext(10.0f, 2.0f));
		TestEqual(TEXT("AffectedByEchoEfficiency=true multiplies reaction damage"), Result.ImmediateDamageApplied, 50.0f);
	}

	FReEchoCsvDataRegistry::LoadAndPublishDefault();
	return true;
}

#endif
