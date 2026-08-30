#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystemComponent.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Core/ReEchoTypes.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Graybox/ReEchoEchoActor.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Recording/ReEchoPlaybackComponent.h"
#include "Recording/ReEchoRecorderComponent.h"
#include "Run/ReEchoRunSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRecordingInterpolationTest,
                                 "ReEcho.Recording.InterpolatesAndHolds",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRecordingInterpolationTest::RunTest(const FString& Parameters)
{
	FReEchoRecording Recording;
	FReEchoPositionSample Start;
	Start.Time = 0.f;
	Start.Position = FVector::ZeroVector;
	FReEchoPositionSample End;
	End.Time = 1.f;
	End.Position = FVector(100.f, 0.f, 0.f);
	Recording.Positions = {Start, End};
	TestTrue("Midpoint is linearly interpolated",
	         Recording.EvaluatePosition(0.5f).Equals(FVector(50.f, 0.f, 0.f), KINDA_SMALL_NUMBER));
	TestTrue("Playback holds its final position",
	         Recording.EvaluatePosition(30.f).Equals(End.Position, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossPlaybackLoopTest,
	                             "ReEcho.Recording.BossPlaybackLoopsEveryEncounterDuration",
	                             EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoBossPlaybackLoopTest::RunTest(const FString& Parameters)
{
	const float LoopDuration = GetDefault<UReEchoBalanceSettings>()->EncounterDuration;
	FReEchoRecording Recording;
	FReEchoPositionSample Start;
	Start.Time = 0.0f;
	Start.Position = FVector::ZeroVector;
	FReEchoPositionSample End;
	End.Time = LoopDuration;
	End.Position = FVector(300.0f, 0.0f, 0.0f);
	Recording.Positions = {Start, End};
	FReEchoSkillEvent EarlySkill;
	EarlySkill.Time = 0.1f;
	EarlySkill.SkillId = TEXT("Early");
	FReEchoSkillEvent LateSkill;
	LateSkill.Time = LoopDuration - 0.1f;
	LateSkill.SkillId = TEXT("Late");
	Recording.Skills = {EarlySkill, LateSkill};

	UReEchoPlaybackComponent* Playback = NewObject<UReEchoPlaybackComponent>(GetTransientPackage());
	Playback->LoadRecording(Recording);
	Playback->SetLoopDuration(LoopDuration);
	Playback->AdvancePlayback(LoopDuration - 0.2f);
	TestEqual(TEXT("Only the early skill has fired before the first cycle tail"), Playback->GetNextSkillIndexForTests(), 1);
	Playback->AdvancePlayback(LoopDuration + 0.05f);
	TestEqual(TEXT("Crossing the boundary enters cycle one"), Playback->GetPlaybackCycleForTests(), int64(1));
	TestEqual(TEXT("The late tail is completed and the new cycle waits for its early event"),
	          Playback->GetNextSkillIndexForTests(),
	          0);
	Playback->AdvancePlayback(LoopDuration + 0.15f);
	TestEqual(TEXT("The early event is replayed once in cycle one"), Playback->GetNextSkillIndexForTests(), 1);
	Playback->AdvancePlayback(2.0f * LoopDuration + 0.15f);
	TestEqual(TEXT("The second boundary enters cycle two"), Playback->GetPlaybackCycleForTests(), int64(2));
	TestEqual(TEXT("The early event is replayed once in cycle two"), Playback->GetNextSkillIndexForTests(), 1);
	TestTrue(TEXT("Boss position wraps to the matching local recording time"),
	         Playback->ResolvePlaybackTimeForTests(2.0f * LoopDuration + 0.5f) == 0.5f);

	UReEchoPlaybackComponent* RestoredPlayback = NewObject<UReEchoPlaybackComponent>(GetTransientPackage());
	RestoredPlayback->LoadRecording(Recording);
	RestoredPlayback->SetLoopDuration(LoopDuration);
	RestoredPlayback->AdvancePlayback(2.0f * LoopDuration + 0.15f);
	TestEqual(TEXT("A restored Echo starts directly in the saved cycle"),
	          RestoredPlayback->GetPlaybackCycleForTests(),
	          int64(2));
	TestEqual(TEXT("A restored Echo only catches up the current cycle"),
	          RestoredPlayback->GetNextSkillIndexForTests(),
	          1);
	RestoredPlayback->AdvancePlayback(2.0f * LoopDuration + 0.15f);
	TestEqual(TEXT("Paused/repeated time does not duplicate the current skill"),
	          RestoredPlayback->GetNextSkillIndexForTests(),
	          1);

	UReEchoPlaybackComponent* OrdinaryPlayback = NewObject<UReEchoPlaybackComponent>(GetTransientPackage());
	OrdinaryPlayback->LoadRecording(Recording);
	OrdinaryPlayback->AdvancePlayback(2.0f * LoopDuration + 0.15f);
	TestEqual(TEXT("Ordinary playback consumes its events once"), OrdinaryPlayback->GetNextSkillIndexForTests(), 2);
	TestEqual(TEXT("Ordinary playback remains in non-looping mode"),
	          OrdinaryPlayback->GetPlaybackCycleForTests(),
	          int64(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoMartyrDefeatRetirementTest,
	                             "ReEcho.Recording.MartyrEchoDefeatRequestsRetirementOnce",
	                             EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoMartyrDefeatRetirementTest::RunTest(const FString& Parameters)
{
	AReEchoEchoActor* OrdinaryEcho = NewObject<AReEchoEchoActor>(GetTransientPackage());
	OrdinaryEcho->NotifyDefeated(EReEchoDamageSource::Enemy);
	TestFalse(TEXT("An ordinary Echo does not request retirement"), OrdinaryEcho->IsRetirementPending());

	AReEchoEchoActor* MartyrEcho = NewObject<AReEchoEchoActor>(GetTransientPackage());
	FReEchoCardRuleSnapshot MartyrRules;
	MartyrRules.bEchoesCanAttack = false;
	MartyrRules.bRetireEchoOnDefeat = true;
	MartyrEcho->ConfigureCardRules(MartyrRules, FReEchoStatBlock{});
	MartyrEcho->NotifyDefeated(EReEchoDamageSource::Enemy);
	TestTrue(TEXT("A defeated Martyr Echo requests world retirement"), MartyrEcho->IsRetirementPending());
	MartyrEcho->NotifyDefeated(EReEchoDamageSource::Enemy);
	TestTrue(TEXT("A duplicate defeat notification remains idempotent"), MartyrEcho->IsRetirementPending());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRecordingLocksInitialWeaponTest,
                                 "ReEcho.Recording.LocksInitialWeapon",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRecordingLocksInitialWeaponTest::RunTest(const FString& Parameters)
{
	UReEchoRecorderComponent* Recorder = NewObject<UReEchoRecorderComponent>(GetTransientPackage());

	FReEchoBuildSnapshot InitialBuild;
	InitialBuild.WeaponId = TEXT("W_J_01");
	Recorder->BeginRecording(1, TEXT("TestArena"), 1337, InitialBuild);
	const FReEchoRecording Recording = Recorder->FinishRecording(10.0f);
	TestEqual(TEXT("Recording keeps the weapon chosen before the run"),
	          Recording.BuildSnapshot.WeaponId,
	          FName(TEXT("W_J_01")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPlayerGasStructureTest,
                                 "ReEcho.GAS.PlayerAbilityStructure",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPlayerGasStructureTest::RunTest(const FString& Parameters)
{
	const AReEchoPlayerPawn* PlayerDefault = GetDefault<AReEchoPlayerPawn>();
	TestNotNull(TEXT("Player default object has an ability system"), PlayerDefault->GetAbilitySystemComponent());
	TestTrue(TEXT("Player implements the ability system interface"),
	         AReEchoPlayerPawn::StaticClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()));

	const TArray<UClass*> AbilityClasses = {UReEchoBasicAttackAbility::StaticClass(),
	                                        UReEchoActiveAttackAbility::StaticClass()};
	for (const UClass* AbilityClass : AbilityClasses)
	{
		TestTrue(TEXT("Player ability derives from UGameplayAbility"),
		         AbilityClass->IsChildOf(UGameplayAbility::StaticClass()));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoFinalBossVictoryTest,
                                 "ReEcho.Run.FinalBossRequiresKill",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoFinalBossVictoryTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* TimedOutRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	TimedOutRun->EncounterIndex = GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount();
	TimedOutRun->CompleteEncounter(FReEchoRecording(), true, false);
	TestEqual(TEXT("Final encounter timeout is a failure"), TimedOutRun->Phase, EReEchoRunPhase::Failed);

	UReEchoRunSubsystem* VictoriousRun = NewObject<UReEchoRunSubsystem>(GameInstance);
	VictoriousRun->EncounterIndex = GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount();
	VictoriousRun->CompleteEncounter(FReEchoRecording(), true, true);
	TestEqual(TEXT("Defeating the final boss enters summary"), VictoriousRun->Phase, EReEchoRunPhase::Summary);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponConfigurationTest,
                                 "ReEcho.Data.WeaponsAreValid",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponConfigurationTest::RunTest(const FString& Parameters)
{
	const UReEchoBalanceSettings* Settings = GetDefault<UReEchoBalanceSettings>();
	const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
	if (!TestTrue(TEXT("Default CSV data loads"), LoadResult.bSuccess))
	{
		AddError(LoadResult.FormatIssues());
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("CSV snapshot is available"), Snapshot.IsValid()))
	{
		return false;
	}

	AReEchoPlayerPawn* CharacterPawn = NewObject<AReEchoPlayerPawn>(GetTransientPackage());
	const TArray<FName> CharacterIds = {TEXT("J_HEART"), TEXT("J_SPADE"), TEXT("J_CLOVER"), TEXT("J_DIAMOND")};
	for (const FName CharacterId : CharacterIds)
	{
		TestTrue(TEXT("Configured player character texture exists"), CharacterPawn->ConfigureCharacter(CharacterId));
	}
	TestTrue(TEXT("Default character id resolves"), CharacterPawn->ConfigureCharacter(Settings->DefaultCharacterId));

	AReEchoEchoActor* EchoActor = NewObject<AReEchoEchoActor>(GetTransientPackage());
	for (const FName CharacterId : CharacterIds)
	{
		TestTrue(TEXT("Configured echo texture exists"), EchoActor->ConfigureEchoAppearance(CharacterId));
	}

	const TArray<FReEchoCsvWeaponRow> StartWeapons = Snapshot->GetStartSelectableWeapons();
	TestEqual(TEXT("CSV has exactly four start-selectable weapons"), StartWeapons.Num(), 4);
	if (StartWeapons.Num() == 4)
	{
		TestEqual(TEXT("First start weapon is the scythe"), StartWeapons[0].Id, FName(TEXT("W_J_04")));
		TestEqual(TEXT("Second start weapon is the longsword"), StartWeapons[1].Id, FName(TEXT("W_J_01")));
		TestEqual(TEXT("Third start weapon is the bow"), StartWeapons[2].Id, FName(TEXT("W_J_08")));
		TestEqual(TEXT("Fourth start weapon is the gun"), StartWeapons[3].Id, FName(TEXT("W_J_09")));
	}
	const FReEchoCsvWeaponRow* Slot1Weapon = Snapshot->FindWeaponByInputSlot(EReEchoInputSlot::Slot1);
	const FReEchoCsvWeaponRow* Slot2Weapon = Snapshot->FindWeaponByInputSlot(EReEchoInputSlot::Slot2);
	const FReEchoCsvWeaponRow* Slot3Weapon = Snapshot->FindWeaponByInputSlot(EReEchoInputSlot::Slot3);
	TestTrue(TEXT("Hotkey 1 has a CSV weapon"), Slot1Weapon != nullptr);
	TestTrue(TEXT("Hotkey 2 has a CSV weapon"), Slot2Weapon != nullptr);
	TestTrue(TEXT("Hotkey 3 has a CSV weapon"), Slot3Weapon != nullptr);
	if (Slot1Weapon && Slot2Weapon && Slot3Weapon)
	{
		TestEqual(TEXT("Hotkey 1 resolves to W_J_04"), Slot1Weapon->Id, FName(TEXT("W_J_04")));
		TestEqual(TEXT("Hotkey 2 resolves to W_J_01"), Slot2Weapon->Id, FName(TEXT("W_J_01")));
		TestEqual(TEXT("Hotkey 3 resolves to W_J_08"), Slot3Weapon->Id, FName(TEXT("W_J_08")));
	}

	const FReEchoCsvWeaponRow* Scythe = Snapshot->FindEnabledWeapon(TEXT("W_J_04"));
	TestTrue(TEXT("W_J_04 is an enabled concrete weapon"), Scythe != nullptr);
	if (Scythe)
	{
		TestEqual(TEXT("W_J_04 uses Scythe type"), Scythe->WeaponTypeId, FName(TEXT("Scythe")));
		TestEqual(TEXT("W_J_04 uses the Scythe pattern"), Scythe->AttackPatternId, FName(TEXT("Pattern.ScytheSweep")));
		TestTrue(TEXT("W_J_04 is start selectable"), Scythe->bStartSelectable);
	}

	int32 EnabledCoreCount = 0;
	int32 DisabledUnnamedCount = 0;
	bool bHasGenericCore = false;
	for (const TPair<FName, FReEchoCsvPartRow>& PartPair : Snapshot->Parts)
	{
		const FReEchoCsvPartRow& Part = PartPair.Value;
		if (Part.PartId == TEXT("None") && !Part.bEnabled)
		{
			++DisabledUnnamedCount;
		}
		if (Part.bEnabled && Part.SlotTypeId == TEXT("Core"))
		{
			++EnabledCoreCount;
		}
		if (Part.bEnabled && Part.PartId == TEXT("P_CORE_FLAME"))
		{
			bHasGenericCore = true;
		}
	}
	TestEqual(TEXT("Four-weapon slot audit keeps 48 source rows"), Snapshot->Parts.Num(), 48);
	TestEqual(TEXT("Retained four-weapon audit has no unnamed rows"), DisabledUnnamedCount, 0);
	TestTrue(TEXT("At least six generic cores are enabled"), EnabledCoreCount >= 6);
	TestTrue(TEXT("A generic core is enabled"), bHasGenericCore);

	return true;
}
#endif
