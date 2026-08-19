#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSaveSnapshotTest,
                                 "ReEcho.Run.SaveSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSaveSnapshotTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
	Source->TimeShards = 45;
	Source->InventoryItems.Add(TEXT("SHOP_OLD_COIN"));
	Source->OwnedPartIds.Add(TEXT("P_CORE_FLAME"));
	Source->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_1_01"));
	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	Recording.EncounterIndex = 1;
	Recording.BuildSnapshot = Source->CurrentBuild;
	TestEqual(TEXT("Completed encounter stages a pending echo"),
	          Source->StagePendingRecording(Recording),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Pending echo stores into a free slot"),
	          Source->StorePendingRecording(),
	          EReEchoEchoStorageResult::Success);
	TestEqual(
	    TEXT("Specific single replay unlocks"), Source->SetSpecificReplayLimit(1), EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Stored echo is selected for the next encounter"),
	          Source->SetSelectedReplayIds({Recording.Id}),
	          EReEchoEchoStorageResult::Success);
	Source->BeginEncounter();

	UReEchoRunSaveGame* Snapshot = Source->CreateSaveSnapshot();
	TestNotNull(TEXT("A save snapshot is created"), Snapshot);
	TestTrue(TEXT("A save snapshot persists a non-zero card-offer seed"), Snapshot->TraitOfferSeed != 0);
	TestEqual(TEXT("Mid-encounter save resumes before that encounter"), Snapshot->EncounterIndex, 0);
	TestEqual(TEXT("Mid-encounter phase normalizes to planning"), Snapshot->SavedPhase, EReEchoRunPhase::Planning);

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	TestTrue(TEXT("Compatible save snapshot restores"), Restored->RestoreSaveSnapshot(*Snapshot));
	TestEqual(TEXT("Card-offer seed restores"),
	          Restored->CreateSaveSnapshot()->TraitOfferSeed,
	          Snapshot->TraitOfferSeed);
	TestEqual(TEXT("Time Shards restore"), Restored->TimeShards, 45);
	TestTrue(TEXT("Inventory restores"), Restored->InventoryItems.Contains(TEXT("SHOP_OLD_COIN")));
	TestTrue(TEXT("Weapon-part ownership restores separately"), Restored->OwnedPartIds.Contains(TEXT("P_CORE_FLAME")));
	TestEqual(TEXT("Selected character restores"), Restored->CurrentBuild.CharacterId, FName(TEXT("J_SPADE")));
	TestEqual(TEXT("Saved current weapon restores"), Restored->CurrentBuild.WeaponId, FName(TEXT("W_J_02")));
	TestEqual(TEXT("Build cards restore"), Restored->CurrentBuild.CardState.OwnedCardIds.Num(), 1);
	const TArray<FReEchoRecording> RestoredRecordings = Restored->GetEchoRecordings(1);
	TestEqual(TEXT("Legacy facade resolves one echo from the new state"), RestoredRecordings.Num(), 1);
	if (RestoredRecordings.Num() == 1)
	{
		TestEqual(TEXT("Selected stored echo restores"), RestoredRecordings[0].Id, Recording.Id);
	}
	const FReEchoEchoStorageSummary RestoredStorage = Restored->GetEchoStorageSummary();
	TestEqual(TEXT("Stored echo slot restores"), RestoredStorage.StoredEchoes.Num(), 1);
	TestEqual(TEXT("Specific replay limit restores"), RestoredStorage.SpecificReplayLimit, 1);
	TestEqual(TEXT("Storage capacity restores"), RestoredStorage.StorageCapacity, 3);
	TestFalse(TEXT("A finalized decision leaves no pending echo"), RestoredStorage.bHasPendingRecording);
	TestTrue(TEXT("Rolling latest echo restores independently"), RestoredStorage.bHasLatestCompletedRecording);

	FReEchoEncounterRuntimeState EncounterState;
	EncounterState.bValid = true;
	EncounterState.EncounterTime = 12.5f;
	EncounterState.NextScheduledSpawnEventIndex = 4;
	EncounterState.RangedBurstWindowRemainingSeconds = {0.9f, 0.35f};
	EncounterState.SpawnResolveSequence = 7;
	EncounterState.ReservedSpawnLocations = {FVector(600.0f, 20.0f, 50.0f)};
	FReEchoPendingSpawnBatchState PendingBatch;
	PendingBatch.WaveId = TEXT("Encounter.2.Wave.2");
	PendingBatch.EnemyRole = TEXT("Ranged");
	PendingBatch.EnemyId = TEXT("M_RABBIT");
	PendingBatch.Locations = {FVector(700.0f, 30.0f, 50.0f)};
	EncounterState.PendingSpawnBatches.Add(PendingBatch);
	EncounterState.PlayerHealth = 63.0f;
	EncounterState.PlayerStats.HpMax = 95.0f;
	EncounterState.PlayerStats.Block = 1;
	EncounterState.PlayerVelocity = FVector(25.0f, -10.0f, 0.0f);
	EncounterState.PlayerTransform.SetLocation(FVector(120.0f, -80.0f, 112.0f));
	EncounterState.ActiveRecording.EncounterIndex = Source->EncounterIndex;
	EncounterState.ActiveRecording.BuildSnapshot = Source->CurrentBuild;
	EncounterState.bBossPostEchoPhaseTriggered = true;
	FReEchoEnemyRuntimeState EnemyState;
	EnemyState.Kind = 2;
	EnemyState.EnemyId = TEXT("M_RABBIT");
	EnemyState.SpawnIndex = 4;
	EnemyState.CurrentHealth = 7.0f;
	EnemyState.AttackCooldown = 0.65f;
	EnemyState.FuseRemaining = 0.4f;
	EnemyState.bBomberFuseActive = true;
	EnemyState.HitReactionRemaining = 0.12f;
	EnemyState.KnockbackVelocity = FVector(45.0f, -12.0f, 0.0f);
	EnemyState.AttackSequence = 9;
	EnemyState.bSelfDestructCommitted = false;
	EnemyState.bHasLogicSnapshot = true;
	EnemyState.LogicSnapshot.Archetype = EReEchoEnemyArchetype::Boss;
	EnemyState.LogicSnapshot.BossCurrentAbilityId = TEXT("BOSS_BLINK_SLAM");
	EnemyState.LogicSnapshot.BossActionPhase = EReEchoBossActionPhase::Windup;
	EnemyState.LogicSnapshot.BossActionPhaseRemainingSeconds = 0.45f;
	EnemyState.LogicSnapshot.BossEncounterElapsedSeconds = 31.0f;
	EncounterState.Enemies.Add(EnemyState);
	UReEchoRunSaveGame* SuspendedSnapshot = Source->CreateSaveSnapshot(&EncounterState);
	TestEqual(
	    TEXT("Explicit quit preserves encounter phase"), SuspendedSnapshot->SavedPhase, EReEchoRunPhase::Encounter);
	TestEqual(TEXT("Explicit quit preserves current encounter index"), SuspendedSnapshot->EncounterIndex, 1);
	TestEqual(
	    TEXT("Explicit quit preserves encounter time"), SuspendedSnapshot->EncounterRuntimeState.EncounterTime, 12.5f);
	TArray<uint8> SerializedSave;
	TestTrue(TEXT("Suspended encounter serializes through SaveGame archive"),
	         UGameplayStatics::SaveGameToMemory(SuspendedSnapshot, SerializedSave));
	const UReEchoRunSaveGame* DeserializedSnapshot =
	    Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromMemory(SerializedSave));
	TestNotNull(TEXT("Suspended encounter deserializes through SaveGame archive"), DeserializedSnapshot);
	if (DeserializedSnapshot)
	{
		TestEqual(TEXT("Serialized encounter time survives round trip"),
		          DeserializedSnapshot->EncounterRuntimeState.EncounterTime,
		          12.5f);
		TestEqual(TEXT("Serialized enemy runtime survives round trip"),
		          DeserializedSnapshot->EncounterRuntimeState.Enemies.Num(),
		          1);
		TestEqual(TEXT("Wave scheduler cursor survives round trip"),
		          DeserializedSnapshot->EncounterRuntimeState.NextScheduledSpawnEventIndex,
		          4);
		TestEqual(TEXT("Ranged burst leases survive round trip"),
		          DeserializedSnapshot->EncounterRuntimeState.RangedBurstWindowRemainingSeconds.Num(),
		          2);
		TestEqual(TEXT("Prepared spawn batch survives round trip"),
		          DeserializedSnapshot->EncounterRuntimeState.PendingSpawnBatches.Num(),
		          1);
		TestEqual(TEXT("Spawn resolver sequence survives round trip"),
		          DeserializedSnapshot->EncounterRuntimeState.SpawnResolveSequence,
		          7);
		TestTrue(TEXT("Boss post-echo phase survives round trip"),
		         DeserializedSnapshot->EncounterRuntimeState.bBossPostEchoPhaseTriggered);
		if (DeserializedSnapshot->EncounterRuntimeState.Enemies.Num() == 1)
		{
			const FReEchoEnemyRuntimeState& SerializedEnemy = DeserializedSnapshot->EncounterRuntimeState.Enemies[0];
			TestEqual(TEXT("Enemy attack cooldown survives round trip"), SerializedEnemy.AttackCooldown, 0.65f);
			TestEqual(TEXT("Stable enemy id survives round trip"), SerializedEnemy.EnemyId, FName(TEXT("M_RABBIT")));
			TestEqual(TEXT("Enemy fuse survives round trip"), SerializedEnemy.FuseRemaining, 0.4f);
			TestTrue(TEXT("Enemy fuse-active state survives round trip"), SerializedEnemy.bBomberFuseActive);
			TestEqual(TEXT("Enemy hit reaction survives round trip"), SerializedEnemy.HitReactionRemaining, 0.12f);
			TestEqual(TEXT("Enemy knockback survives round trip"),
			          SerializedEnemy.KnockbackVelocity,
			          FVector(45.0f, -12.0f, 0.0f));
			TestEqual(
			    TEXT("Enemy attack identity sequence survives round trip"), SerializedEnemy.AttackSequence, int64(9));
			TestFalse(TEXT("Enemy one-shot self-destruct state survives round trip"),
			          SerializedEnemy.bSelfDestructCommitted);
			TestTrue(TEXT("Canonical enemy logic snapshot survives round trip"), SerializedEnemy.bHasLogicSnapshot);
			TestEqual(TEXT("Boss active ability survives round trip"),
			          SerializedEnemy.LogicSnapshot.BossCurrentAbilityId,
			          FName(TEXT("BOSS_BLINK_SLAM")));
			TestEqual(TEXT("Boss windup remaining survives round trip"),
			          SerializedEnemy.LogicSnapshot.BossActionPhaseRemainingSeconds,
			          0.45f);
		}
	}

	UGameInstance* SuspendedGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* SuspendedRun = NewObject<UReEchoRunSubsystem>(SuspendedGameInstance);
	TestTrue(TEXT("Suspended encounter snapshot restores"), SuspendedRun->RestoreSaveSnapshot(*SuspendedSnapshot));
	TestTrue(TEXT("Suspended encounter is pending for GameMode restoration"),
	         SuspendedRun->HasPendingEncounterResume());
	const FReEchoEncounterRuntimeState RestoredEncounter = SuspendedRun->ConsumePendingEncounterResume();
	TestEqual(TEXT("Player health restores from suspended encounter"), RestoredEncounter.PlayerHealth, 63.0f);
	TestTrue(TEXT("Restored encounter keeps Boss post-echo phase"), RestoredEncounter.bBossPostEchoPhaseTriggered);
	TestEqual(TEXT("Enemy runtime state restores"), RestoredEncounter.Enemies.Num(), 1);
	TestEqual(TEXT("Wave scheduler cursor restores"), RestoredEncounter.NextScheduledSpawnEventIndex, 4);
	TestEqual(TEXT("Ranged burst leases restore"), RestoredEncounter.RangedBurstWindowRemainingSeconds.Num(), 2);
	TestEqual(TEXT("Prepared spawn batch restores"), RestoredEncounter.PendingSpawnBatches.Num(), 1);
	TestEqual(TEXT("Reserved spawn positions restore"), RestoredEncounter.ReservedSpawnLocations.Num(), 1);
	TestFalse(TEXT("Suspended encounter snapshot is consumed once"), SuspendedRun->HasPendingEncounterResume());

	SuspendedSnapshot->SaveVersion = 8;
	UGameInstance* LegacyProgressGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* LegacyProgressRun = NewObject<UReEchoRunSubsystem>(LegacyProgressGameInstance);
	TestFalse(TEXT("Progressed legacy six-encounter save is explicitly rejected"),
	          LegacyProgressRun->RestoreSaveSnapshot(*SuspendedSnapshot));

	Snapshot->SaveVersion = UReEchoRunSaveGame::CurrentSaveVersion + 1;
	TestFalse(TEXT("Incompatible save version is rejected"), Restored->RestoreSaveSnapshot(*Snapshot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoV10CharacterIdentityMigrationTest,
                                 "ReEcho.Run.SaveV10CharacterIdentityMigratesAcrossSnapshots",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoV10CharacterIdentityMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	UReEchoRunSaveGame* LegacySave = Source->CreateSaveSnapshot();
	LegacySave->SaveVersion = 10;

	auto MakeLegacyRecording = [Source](const FName LegacyCharacterId)
	{
		FReEchoRecording Recording;
		Recording.Id = FGuid::NewGuid();
		Recording.BuildSnapshot = Source->CurrentBuild;
		Recording.BuildSnapshot.CharacterId = LegacyCharacterId;
		Recording.BuildSnapshot.RuleFlags.Add(TEXT("BaseCharacterId"), LegacyCharacterId.ToString());
		return Recording;
	};

	LegacySave->CurrentBuild.CharacterId = TEXT("J_CAT");
	LegacySave->CurrentBuild.RuleFlags.Add(TEXT("BaseCharacterId"), TEXT("J_CAT"));
	LegacySave->bHasPendingRecording = true;
	LegacySave->PendingRecording = MakeLegacyRecording(TEXT("J_CAT"));
	LegacySave->bHasLatestCompletedRecording = true;
	LegacySave->LatestCompletedRecording = MakeLegacyRecording(TEXT("J01"));
	LegacySave->bHasPreviousCompletedRecording = true;
	LegacySave->PreviousCompletedRecording = MakeLegacyRecording(TEXT("J_CAT"));
	LegacySave->StoredEchoes = {MakeLegacyRecording(TEXT("J01"))};
	LegacySave->EncounterRuntimeState.bValid = true;
	LegacySave->EncounterRuntimeState.ActiveRecording = MakeLegacyRecording(TEXT("J_CAT"));

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	TestTrue(TEXT("v10 save migrates every nested legacy character identity"),
	         Restored->RestoreSaveSnapshot(*LegacySave));
	TestEqual(
	    TEXT("Current build migrates J_CAT to J_SPADE"), Restored->CurrentBuild.CharacterId, FName(TEXT("J_SPADE")));
	TestEqual(TEXT("Base character rule flag migrates to J_SPADE"),
	          Restored->CurrentBuild.RuleFlags.FindRef(TEXT("BaseCharacterId")),
	          FString(TEXT("J_SPADE")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoV8CardMigrationTest,
                                 "ReEcho.Run.SaveV8CardIdsMigrateByMeaning",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoV8CardMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
	UReEchoRunSaveGame* LegacySave = Source->CreateSaveSnapshot();
	LegacySave->SaveVersion = 8;
	LegacySave->CurrentBuild.CardState = {};
	LegacySave->CurrentBuild.Cards = {TEXT("G_1_03"), TEXT("G_1_04"), TEXT("G_1_05"), TEXT("G_1_08"), TEXT("G_2_01")};

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	TestTrue(TEXT("v8 save restores through semantic card migration"), Restored->RestoreSaveSnapshot(*LegacySave));
	TestEqual(TEXT("Only cards with a current semantic equivalent migrate"),
	          Restored->CurrentBuild.CardState.OwnedCardIds.Num(),
	          3);
	TestTrue(TEXT("Legacy physical card migrates to the new physical ID"),
	         Restored->CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_1_03")));
	TestTrue(TEXT("Legacy elemental card migrates to the new elemental ID"),
	         Restored->CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_1_04")));
	TestTrue(TEXT("Legacy movement card migrates to the new movement ID"),
	         Restored->CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_1_07")));
	TestFalse(TEXT("Removed tier-two card is not retained"),
	          Restored->CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_2_01")));
	TestTrue(TEXT("Legacy card array is cleared after migration"), Restored->CurrentBuild.Cards.IsEmpty());
	TestTrue(TEXT("Migrated build pins the current card data domain"),
	         !Restored->CurrentBuild.CardState.DomainRevision.IsEmpty());

	UReEchoRunSaveGame* EmergencyOnlySave = DuplicateObject<UReEchoRunSaveGame>(LegacySave, GetTransientPackage());
	EmergencyOnlySave->CurrentBuild.CardState = {};
	EmergencyOnlySave->CurrentBuild.Cards = {TEXT("G_1_03")};
	UGameInstance* EmergencyGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* EmergencyRestored = NewObject<UReEchoRunSubsystem>(EmergencyGameInstance);
	TestTrue(TEXT("v8 emergency-only save remains loadable"),
	         EmergencyRestored->RestoreSaveSnapshot(*EmergencyOnlySave));
	TestTrue(TEXT("Removed emergency block does not gain the new G_1_03 meaning"),
	         EmergencyRestored->CurrentBuild.CardState.OwnedCardIds.IsEmpty());
	return true;
}

#endif
