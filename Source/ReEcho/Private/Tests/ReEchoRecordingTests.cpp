#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystem/ReEchoPlayerAbilities.h"
#include "AbilitySystemComponent.h"
#include "Player/ReEchoPlayerPawn.h"
#include "Core/ReEchoTypes.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Engine/GameInstance.h"
#include "Graybox/ReEchoEchoActor.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoWeaponTimelineRecordingTest,
                                 "ReEcho.Recording.CapturesWeaponTimeline",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponTimelineRecordingTest::RunTest(const FString& Parameters)
{
	UReEchoRecorderComponent* Recorder = NewObject<UReEchoRecorderComponent>(GetTransientPackage());

	FReEchoBuildSnapshot InitialBuild;
	InitialBuild.WeaponId = TEXT("W_J_02");
	Recorder->BeginRecording(1, TEXT("TestArena"), 1337, InitialBuild);
	Recorder->RecordWeaponChange(4.0f, TEXT("W_J_01"));
	Recorder->RecordWeaponChange(5.0f, TEXT("W_J_01"));
	Recorder->RecordWeaponChange(9.5f, TEXT("W_J_03"));

	const FReEchoRecording Recording = Recorder->FinishRecording(10.0f);
	TestEqual(TEXT("Initial and two distinct changes are recorded"), Recording.WeaponChanges.Num(), 3);
	TestEqual(
	    TEXT("Timeline starts with the initial weapon"), Recording.WeaponChanges[0].WeaponId, FName(TEXT("W_J_02")));
	TestEqual(TEXT("Sword switch keeps its encounter time"), Recording.WeaponChanges[1].Time, 4.0f);
	TestEqual(
	    TEXT("Duplicate weapon selection is ignored"), Recording.WeaponChanges[2].WeaponId, FName(TEXT("W_J_03")));
	TestEqual(TEXT("Build snapshot retains the final weapon"), Recording.BuildSnapshot.WeaponId, FName(TEXT("W_J_03")));
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
	                                        UReEchoActiveAttackAbility::StaticClass(),
	                                        UReEchoSelectWeaponSlot1Ability::StaticClass(),
	                                        UReEchoSelectWeaponSlot2Ability::StaticClass(),
	                                        UReEchoSelectWeaponSlot3Ability::StaticClass()};
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
                                 "ReEcho.Config.WeaponsAreValid",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoWeaponConfigurationTest::RunTest(const FString& Parameters)
{
	const UReEchoBalanceSettings* Settings = GetDefault<UReEchoBalanceSettings>();
	TestTrue(TEXT("At least one weapon is configured"), !Settings->Weapons.IsEmpty());

	AReEchoPlayerPawn* CharacterPawn = NewObject<AReEchoPlayerPawn>(GetTransientPackage());
	const TArray<FName> CharacterIds = {
		TEXT("J_CAT"), TEXT("J_HEART"), TEXT("J_SPADE"), TEXT("J_CLOVER"), TEXT("J_DIAMOND")};
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

	TSet<EReEchoWeaponSlot> Slots;
	TSet<FName> WeaponIds;
	for (const FReEchoWeaponConfig& Weapon : Settings->Weapons)
	{
		TestTrue(TEXT("Weapon slot is assigned"), Weapon.Slot != EReEchoWeaponSlot::None);
		TestTrue(TEXT("Weapon id is assigned"), !Weapon.WeaponId.IsNone());
		TestTrue(TEXT("Weapon interval is positive"), Weapon.Interval > 0.0f);
		TestTrue(TEXT("Weapon range is positive"), Weapon.Range > 0.0f);
		TestFalse(TEXT("Weapon slots are unique"), Slots.Contains(Weapon.Slot));
		TestFalse(TEXT("Weapon ids are unique"), WeaponIds.Contains(Weapon.WeaponId));
		Slots.Add(Weapon.Slot);
		WeaponIds.Add(Weapon.WeaponId);
	}

	return true;
}
#endif
