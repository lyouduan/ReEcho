#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoPlayerProgressSaveGame.h"
#include "Run/ReEchoRunSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoPlayerProgressSaveTest,
                                 "ReEcho.Run.PlayerProgress.Stage01To02Cg",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoPlayerProgressSaveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UReEchoPlayerProgressSaveGame* FreshProgress = NewObject<UReEchoPlayerProgressSaveGame>();
	TestFalse(TEXT("A new account has not viewed Stage01To02 CG"), FreshProgress->bHasViewedStage01To02Cg);
	FreshProgress->bHasViewedStage01To02Cg = true;
	FreshProgress->PreferredDifficulty = EReEchoRunDifficulty::Nightmare;
	TArray<uint8> SerializedProgress;
	TestTrue(TEXT("Account progress serializes independently from a run slot"),
	         UGameplayStatics::SaveGameToMemory(FreshProgress, SerializedProgress));
	const UReEchoPlayerProgressSaveGame* RestoredProgress =
	    Cast<UReEchoPlayerProgressSaveGame>(UGameplayStatics::LoadGameFromMemory(SerializedProgress));
	TestNotNull(TEXT("Account progress deserializes"), RestoredProgress);
	if (RestoredProgress)
	{
		TestEqual(TEXT("Player progress version survives serialization"),
		          RestoredProgress->SaveVersion,
		          UReEchoPlayerProgressSaveGame::CurrentSaveVersion);
		TestTrue(TEXT("Viewed CG state survives serialization"), RestoredProgress->bHasViewedStage01To02Cg);
		TestEqual(TEXT("Next-run difficulty survives serialization"),
		          RestoredProgress->PreferredDifficulty,
		          EReEchoRunDifficulty::Nightmare);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoSaveSnapshotTest,
                                 "ReEcho.Run.SaveSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoSaveSnapshotTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	TestFalse(TEXT("Save slot selection rejects a negative index"), Source->SelectSaveSlot(-1));
	TestFalse(TEXT("Save slot selection rejects an out-of-range index"), Source->SelectSaveSlot(3));
	TestTrue(TEXT("The third save slot can be selected independently"), Source->SelectSaveSlot(2));
	TestEqual(TEXT("The selected save slot is authoritative"), Source->GetActiveSaveSlotIndex(), 2);
	TestTrue(TEXT("The selected slot owns a stable screenshot path"),
	         Source->GetActiveSaveSlotPreviewPath().EndsWith(TEXT("SaveScreenshots/ReEchoRunSlot3.png")));
	Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Source->TimeShards = 45;
	Source->InventoryItems.Add(TEXT("SHOP_OLD_COIN"));
	Source->OwnedPartIds.Add(TEXT("P_CORE_FLAME"));
	Source->OwnedWeaponIds.Add(TEXT("W_J_08"));
	Source->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_1_01"));
	Source->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_3_02"));
	FReEchoRecording Recording;
	Recording.Id = FGuid::NewGuid();
	Recording.EncounterIndex = 1;
	Recording.BuildSnapshot = Source->CurrentBuild;
	TestEqual(TEXT("Completed encounter stages a pending echo"),
	          Source->StagePendingRecording(Recording),
	          EReEchoEchoStorageResult::Success);
	TestEqual(TEXT("Pending echo becomes the time anchor"),
	          Source->StorePendingRecordingAsTimeAnchor(),
	          EReEchoEchoStorageResult::Success);
	Source->BeginEncounter();
	const int32 EnemyDrop = Source->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), 17);
	TestTrue(TEXT("A death drop is available for save roundtrip"), EnemyDrop >= 2 && EnemyDrop <= 3);
	TestEqual(TEXT("Uncollected death drops do not change the saved balance"), Source->TimeShards, 45);

	UReEchoRunSaveGame* Snapshot = Source->CreateSaveSnapshot();
	TestNotNull(TEXT("A save snapshot is created"), Snapshot);
	TestEqual(TEXT("A save snapshot persists its logical slot"), Snapshot->LogicalSlotIndex, 2);
	TestTrue(TEXT("A save snapshot persists an actual UTC save time"), Snapshot->SavedAtUtcTicks > 0);
	TestEqual(TEXT("A save snapshot persists its screenshot file name"),
	          Snapshot->PreviewScreenshotFileName,
	          FString(TEXT("SaveScreenshots/ReEchoRunSlot3.png")));
	TestTrue(TEXT("A save snapshot persists a non-zero unified run seed"), Snapshot->RunSeed != 0);
	TestTrue(TEXT("A save snapshot persists a non-zero card-offer seed"), Snapshot->TraitOfferSeed != 0);
	TestTrue(TEXT("A save snapshot persists a non-zero enemy-reward seed"), Snapshot->EnemyShardDropSeed != 0);
	TestEqual(
	    TEXT("A save snapshot persists processed enemy reward keys"), Snapshot->RewardedEnemyShardDropKeys.Num(), 1);
	TestEqual(TEXT("Mid-encounter save resumes before that encounter"), Snapshot->EncounterIndex, 0);
	TestEqual(TEXT("Mid-encounter phase normalizes to planning"), Snapshot->SavedPhase, EReEchoRunPhase::Planning);

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	TestTrue(TEXT("Compatible save snapshot restores"), Restored->RestoreSaveSnapshot(*Snapshot));
	TestEqual(
	    TEXT("Card-offer seed restores"), Restored->CreateSaveSnapshot()->TraitOfferSeed, Snapshot->TraitOfferSeed);
	const UReEchoRunSaveGame* RestoredSnapshot = Restored->CreateSaveSnapshot();
	TestEqual(TEXT("Unified run seed restores"), RestoredSnapshot->RunSeed, Snapshot->RunSeed);
	TestEqual(TEXT("Enemy-reward seed restores"), RestoredSnapshot->EnemyShardDropSeed, Snapshot->EnemyShardDropSeed);
	TestEqual(TEXT("Processed enemy reward keys restore"), RestoredSnapshot->RewardedEnemyShardDropKeys.Num(), 1);
	TestEqual(TEXT("Time Shards restore without an uncollected drop"), Restored->TimeShards, 45);
	TestTrue(TEXT("Inventory restores"), Restored->InventoryItems.Contains(TEXT("SHOP_OLD_COIN")));
	TestTrue(TEXT("Weapon-part ownership restores separately"), Restored->OwnedPartIds.Contains(TEXT("P_CORE_FLAME")));
	TestTrue(TEXT("Starting weapon ownership restores"), Restored->OwnedWeaponIds.Contains(TEXT("W_J_01")));
	TestTrue(TEXT("Additional weapon ownership restores"), Restored->OwnedWeaponIds.Contains(TEXT("W_J_08")));
	TestEqual(TEXT("Selected character restores"), Restored->CurrentBuild.CharacterId, FName(TEXT("J_SPADE")));
	TestEqual(TEXT("Saved current weapon restores"), Restored->CurrentBuild.WeaponId, FName(TEXT("W_J_01")));
	TestEqual(TEXT("Build cards restore"), Restored->CurrentBuild.CardState.OwnedCardIds.Num(), 2);
	const TArray<FReEchoRecording> RestoredRecordings = Restored->GetEchoRecordings(1);
	TestEqual(TEXT("Legacy facade resolves one echo from the new state"), RestoredRecordings.Num(), 1);
	if (RestoredRecordings.Num() == 1)
	{
		TestEqual(TEXT("Selected stored echo restores"), RestoredRecordings[0].Id, Recording.Id);
	}
	const FReEchoEchoStorageSummary RestoredStorage = Restored->GetEchoStorageSummary();
	TestTrue(TEXT("Time anchor restores"), RestoredStorage.bHasTimeAnchor);
	TestEqual(TEXT("Time anchor id restores"), RestoredStorage.TimeAnchorRecording.RecordingId, Recording.Id);
	TestFalse(TEXT("A finalized decision leaves no pending echo"), RestoredStorage.bHasPendingRecording);
	TestTrue(TEXT("Rolling latest echo restores independently"), RestoredStorage.bHasLatestCompletedRecording);

	UReEchoRunSaveGame* EasterSnapshot = DuplicateObject<UReEchoRunSaveGame>(Snapshot, GetTransientPackage());
	EasterSnapshot->CurrentBuild.CardState.OwnedCardIds.Add(TEXT("G_4_8"));
	FReEchoCardRuntimeState& EasterRuntime = EasterSnapshot->CurrentBuild.CardState.Runtime;
	EasterRuntime.bHasPreviousEncounterShardIncome = true;
	EasterRuntime.PreviousEncounterGrossShardIncome = 120;
	EasterRuntime.CurrentEncounterGrossShardIncome = 45;
	EasterRuntime.EncounterShardIncomeMultiplier = 1.35f;
	EasterRuntime.ShopCardOfferEncounterIndex = EasterSnapshot->EncounterIndex;
	EasterRuntime.ShopCardOfferRefreshSequence = 0;
	EasterRuntime.ShopCardPackStates.SetNum(3);
	for (int32 PackIndex = 0; PackIndex < EasterRuntime.ShopCardPackStates.Num(); ++PackIndex)
	{
		EasterRuntime.ShopCardPackStates[PackIndex].Tier = PackIndex + 1;
	}
	EasterRuntime.ShopCardPackStates[0].CandidateCardIds = {TEXT("G_4_1")};
	EasterRuntime.ShopCardPackStates[0].OfferHistoryCardIds = {TEXT("G_4_1")};
	EasterRuntime.ShopCardPackStates[0].SlotRefreshUses = {0};
	UGameInstance* EasterGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* EasterRestored = NewObject<UReEchoRunSubsystem>(EasterGameInstance);
	TestTrue(TEXT("A v25 snapshot restores owned Easter cards, their runtime state, and cached shop offers"),
	         EasterRestored->RestoreSaveSnapshot(*EasterSnapshot));
	TestTrue(TEXT("Owned Easter card identity restores"),
	         EasterRestored->CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_4_8")));
	TestEqual(TEXT("Previous encounter gross shard income restores"),
	          EasterRestored->CurrentBuild.CardState.Runtime.PreviousEncounterGrossShardIncome,
	          120);
	TestEqual(TEXT("Current encounter gross shard income restores"),
	          EasterRestored->CurrentBuild.CardState.Runtime.CurrentEncounterGrossShardIncome,
	          45);
	TestEqual(TEXT("Encounter shard multiplier restores"),
	          EasterRestored->CurrentBuild.CardState.Runtime.EncounterShardIncomeMultiplier,
	          1.35f);
	TestEqual(TEXT("Cached Easter shop offer restores"),
	          EasterRestored->CurrentBuild.CardState.Runtime.ShopCardPackStates[0].CandidateCardIds[0],
	          FName(TEXT("G_4_1")));

	UReEchoRunSaveGame* RetiredCurrentWeapon = DuplicateObject<UReEchoRunSaveGame>(Snapshot, GetTransientPackage());
	RetiredCurrentWeapon->CurrentBuild.WeaponId = TEXT("W_J_02");
	UGameInstance* RetiredCurrentGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RetiredCurrentRun = NewObject<UReEchoRunSubsystem>(RetiredCurrentGameInstance);
	TestFalse(TEXT("A save equipped with retired W_J_02 is explicitly incompatible"),
	          RetiredCurrentRun->RestoreSaveSnapshot(*RetiredCurrentWeapon));

	UReEchoRunSaveGame* RetiredOwnedWeapon = DuplicateObject<UReEchoRunSaveGame>(Snapshot, GetTransientPackage());
	RetiredOwnedWeapon->OwnedWeaponIds.Add(TEXT("W_J_07"));
	UGameInstance* RetiredOwnedGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RetiredOwnedRun = NewObject<UReEchoRunSubsystem>(RetiredOwnedGameInstance);
	TestFalse(TEXT("A save owning retired W_J_07 is explicitly incompatible"),
	          RetiredOwnedRun->RestoreSaveSnapshot(*RetiredOwnedWeapon));

	UReEchoRunSaveGame* RetiredRecordingWeapon = DuplicateObject<UReEchoRunSaveGame>(Snapshot, GetTransientPackage());
	RetiredRecordingWeapon->StoredEchoes[0].BuildSnapshot.WeaponId = TEXT("W_J_02");
	UGameInstance* RetiredRecordingGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RetiredRecordingRun = NewObject<UReEchoRunSubsystem>(RetiredRecordingGameInstance);
	TestFalse(TEXT("A stored recording using retired W_J_02 is explicitly incompatible"),
	          RetiredRecordingRun->RestoreSaveSnapshot(*RetiredRecordingWeapon));

	UReEchoRunSaveGame* PreUnifiedSeedSave = DuplicateObject<UReEchoRunSaveGame>(Snapshot, GetTransientPackage());
	PreUnifiedSeedSave->SaveVersion = 23;
	PreUnifiedSeedSave->RunSeed = 0;
	PreUnifiedSeedSave->CurrentBuild.CardState.Runtime.bHasPreviousEncounterShardIncome = true;
	PreUnifiedSeedSave->CurrentBuild.CardState.Runtime.PreviousEncounterGrossShardIncome = 999;
	PreUnifiedSeedSave->CurrentBuild.CardState.Runtime.CurrentEncounterGrossShardIncome = 888;
	PreUnifiedSeedSave->CurrentBuild.CardState.Runtime.EncounterShardIncomeMultiplier = 9.0f;
	UGameInstance* FirstSeedMigrationGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* FirstSeedMigrationRun = NewObject<UReEchoRunSubsystem>(FirstSeedMigrationGameInstance);
	UGameInstance* SecondSeedMigrationGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* SecondSeedMigrationRun = NewObject<UReEchoRunSubsystem>(SecondSeedMigrationGameInstance);
	TestTrue(TEXT("A v23 save without the unified seed remains loadable"),
	         FirstSeedMigrationRun->RestoreSaveSnapshot(*PreUnifiedSeedSave));
	TestTrue(TEXT("The same v23 save can be migrated repeatedly"),
	         SecondSeedMigrationRun->RestoreSaveSnapshot(*PreUnifiedSeedSave));
	const int32 FirstMigratedRunSeed = FirstSeedMigrationRun->CreateSaveSnapshot()->RunSeed;
	const int32 SecondMigratedRunSeed = SecondSeedMigrationRun->CreateSaveSnapshot()->RunSeed;
	TestTrue(TEXT("A legacy save receives a non-zero unified run seed"), FirstMigratedRunSeed != 0);
	TestEqual(TEXT("Legacy unified seed migration is deterministic"), FirstMigratedRunSeed, SecondMigratedRunSeed);
	TestFalse(TEXT("Pre-v25 saves migrate without Easter shard history"),
	          FirstSeedMigrationRun->CurrentBuild.CardState.Runtime.bHasPreviousEncounterShardIncome);
	TestEqual(TEXT("Pre-v25 previous shard income migrates to zero"),
	          FirstSeedMigrationRun->CurrentBuild.CardState.Runtime.PreviousEncounterGrossShardIncome,
	          0);
	TestEqual(TEXT("Pre-v25 current shard income migrates to zero"),
	          FirstSeedMigrationRun->CurrentBuild.CardState.Runtime.CurrentEncounterGrossShardIncome,
	          0);
	TestEqual(TEXT("Pre-v25 shard multiplier migrates to neutral"),
	          FirstSeedMigrationRun->CurrentBuild.CardState.Runtime.EncounterShardIncomeMultiplier,
	          1.0f);

	Snapshot->SaveVersion = 12;
	Snapshot->OwnedWeaponIds.Reset();
	UGameInstance* LegacyWeaponGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* LegacyWeaponRestored = NewObject<UReEchoRunSubsystem>(LegacyWeaponGameInstance);
	TestTrue(TEXT("A pre-v13 save migrates without a weapon ownership field"),
	         LegacyWeaponRestored->RestoreSaveSnapshot(*Snapshot));
	TestTrue(TEXT("A pre-v13 save derives ownership from its equipped weapon"),
	         LegacyWeaponRestored->OwnedWeaponIds.Contains(LegacyWeaponRestored->CurrentBuild.WeaponId));
	Snapshot->SaveVersion = UReEchoRunSaveGame::CurrentSaveVersion;

	Snapshot->SavedPhase = EReEchoRunPhase::LegacyForgeChoice;
	UGameInstance* LegacyForgeGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* LegacyForgeRestored = NewObject<UReEchoRunSubsystem>(LegacyForgeGameInstance);
	TestTrue(TEXT("A legacy Forge phase save remains loadable"), LegacyForgeRestored->RestoreSaveSnapshot(*Snapshot));
	TestEqual(TEXT("A legacy Forge phase migrates to regular card choice"),
	          LegacyForgeRestored->Phase,
	          EReEchoRunPhase::CardChoice);

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
	EncounterState.bHasPhase3SpawnPlan = true;
	FReEchoScheduledSpawnEvent Deferred;
	Deferred.Type = EReEchoScheduledSpawnEventType::Commit;
	Deferred.WaveId = TEXT("Phase3.2.Encounter.8.Wave.1");
	Deferred.EnemyId = TEXT("M_RABBIT");
	Deferred.EnemyRole = TEXT("Ranged");
	Deferred.Count = 3;
	Deferred.EventSeconds = 75.0f;
	Deferred.SpawnSeconds = 75.0f;
	EncounterState.Phase3SpawnEvents.Add(Deferred);
	EncounterState.DeferredPhase3Spawns.Add(Deferred);
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
	EnemyState.LogicSnapshot.bBossOpeningConsumed = true;
	EnemyState.LogicSnapshot.bBossOpeningQueued = true;
	EnemyState.SavedMaxHealth = 25000.0f;
	EnemyState.bPendingSlam = true;
	EnemyState.PendingSlamSeconds = 0.2f;
	EnemyState.PendingSlamIntent.bPhaseOpening = true;
	EnemyState.OpeningRepulseDisplacement = FVector(200.0f, 0.0f, 0.0f);
	EnemyState.OpeningRepulseDuration = 0.3f;
	EnemyState.OpeningRepulseElapsed = 0.15f;
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
		TestEqual(TEXT("Logical slot metadata survives serialization"), DeserializedSnapshot->LogicalSlotIndex, 2);
		TestEqual(TEXT("Screenshot metadata survives serialization"),
		          DeserializedSnapshot->PreviewScreenshotFileName,
		          FString(TEXT("SaveScreenshots/ReEchoRunSlot3.png")));
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
		TestTrue(TEXT("Dedicated phase3 plan flag survives archive"),
		         DeserializedSnapshot->EncounterRuntimeState.bHasPhase3SpawnPlan);
		TestEqual(TEXT("Dedicated phase3 events survive archive"),
		          DeserializedSnapshot->EncounterRuntimeState.Phase3SpawnEvents.Num(),
		          1);
		const TArray<FReEchoScheduledSpawnEvent>& SavedDeferred =
		    DeserializedSnapshot->EncounterRuntimeState.DeferredPhase3Spawns;
		TestEqual(TEXT("Deferred quota survives archive"), SavedDeferred.Num(), 1);
		if (SavedDeferred.Num() == 1)
		{
			TestEqual(TEXT("Only remaining quota is saved"), SavedDeferred[0].Count, 3);
			TestEqual(TEXT("Deferred wave identity survives"), SavedDeferred[0].WaveId, Deferred.WaveId);
		}
		if (DeserializedSnapshot->EncounterRuntimeState.Enemies.Num() == 1)
		{
			const FReEchoEnemyRuntimeState& SerializedEnemy = DeserializedSnapshot->EncounterRuntimeState.Enemies[0];
			TestTrue(TEXT("Opening consumption survives archive"), SerializedEnemy.LogicSnapshot.bBossOpeningConsumed);
			TestTrue(TEXT("Queued opening survives archive"), SerializedEnemy.LogicSnapshot.bBossOpeningQueued);
			TestEqual(TEXT("Phase3 health ceiling survives archive"), SerializedEnemy.SavedMaxHealth, 25000.0f);
			TestTrue(TEXT("Pending landing survives archive"),
			         SerializedEnemy.bPendingSlam && SerializedEnemy.PendingSlamIntent.bPhaseOpening);
			TestEqual(TEXT("Landing remaining time survives archive"), SerializedEnemy.PendingSlamSeconds, 0.2f);
			TestEqual(TEXT("Repulse progress survives archive"), SerializedEnemy.OpeningRepulseElapsed, 0.15f);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoV15ShopCardPageMigrationTest,
                                 "ReEcho.Run.SaveV15ShopCardPageMigratesToTierPacks",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoV15ShopCardPageMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Source->EncounterIndex = 4;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = Source->GetRunDataSnapshot();
	const TArray<FReEchoCardDefinition> TierTwoCards =
	    Snapshot->CardCatalog->GetOfferable(TEXT("Trait"), 2);
	if (!TestTrue(TEXT("Migration fixture has a tier-two catalog card"), !TierTwoCards.IsEmpty()))
	{
		return false;
	}
	UReEchoRunSaveGame* LegacySave = Source->CreateSaveSnapshot();
	LegacySave->SaveVersion = 15;
	LegacySave->CurrentBuild.CardState.Runtime.ShopCardOfferIds = {TierTwoCards[0].Id};
	LegacySave->CurrentBuild.CardState.Runtime.ShopCardPackStates.Reset();

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	TestTrue(TEXT("v15 direct-card shop page remains loadable"), Restored->RestoreSaveSnapshot(*LegacySave));
	const FReEchoWeaponPartShopView MigratedPage = Restored->GetWeaponPartShopView();
	TestEqual(TEXT("The legacy direct-card page rebuilds as three fixed packs"),
	          MigratedPage.CardPackOffers.Num(),
	          ReEchoShopOfferCountPerGroup);
	TestTrue(TEXT("The migrated tier-two pack is available and defers its choices until payment"),
	         MigratedPage.CardPackOffers[1].IsAvailable() && MigratedPage.CardPackOffers[1].Choices.IsEmpty());
	TestTrue(TEXT("The legacy one-card cache is discarded"),
	         Restored->CurrentBuild.CardState.Runtime.ShopCardOfferIds.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoV19ShopCardPackPaymentMigrationTest,
                                 "ReEcho.Run.SaveV19ShopCardPackMigratesToPrepaidState",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoV19ShopCardPackPaymentMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
	Source->EncounterIndex = 4;
	Source->TimeShards = 1000;
	const FReEchoWeaponPartShopView Page = Source->GetWeaponPartShopView();
	if (!TestTrue(TEXT("v19 migration fixture has an available tier-two pack"),
	              Page.CardPackOffers.IsValidIndex(1) && Page.CardPackOffers[1].IsAvailable()))
	{
		return false;
	}
	if (!TestTrue(TEXT("Migration fixture pays its legacy pack"),
	              Source->PurchaseShopCardPackDetailed(2).IsSuccess()))
	{
		return false;
	}
	const FReEchoShopCardPackOffer PaidPack = Source->GetWeaponPartShopView().CardPackOffers[1];
	if (!TestTrue(TEXT("Migration fixture payment generates a candidate"), !PaidPack.Choices.IsEmpty()) ||
	    !TestTrue(TEXT("Migration fixture claims its legacy pack"),
	              Source->ClaimPaidShopCardChoice(PaidPack.Choices[0].ItemId).IsSuccess()))
	{
		return false;
	}
	UReEchoRunSaveGame* LegacySave = Source->CreateSaveSnapshot();
	LegacySave->SaveVersion = 19;
	for (FReEchoShopCardPackRuntimeState& Pack : LegacySave->CurrentBuild.CardState.Runtime.ShopCardPackStates)
	{
		Pack.BasePrice = 0;
		Pack.bPaymentCommitted = false;
	}

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	if (!TestTrue(TEXT("v19 card-pack state restores"), Restored->RestoreSaveSnapshot(*LegacySave)))
	{
		return false;
	}
	const FReEchoWeaponPartShopView MigratedPage = Restored->GetWeaponPartShopView();
	TestEqual(TEXT("A legacy purchased pack remains purchased"),
	          MigratedPage.CardPackOffers[1].Status,
	          EReEchoShopCardPackStatus::Purchased);
	TestTrue(TEXT("A legacy purchased pack migrates to committed payment"),
	         Restored->CurrentBuild.CardState.Runtime.ShopCardPackStates[1].bPaymentCommitted);
	TestTrue(TEXT("A migrated pack lazily receives one stable tier price"), MigratedPage.CardPackOffers[1].Price > 0);
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
	TestFalse(TEXT("Legacy base-character flag is consumed during migration"),
	          Restored->CurrentBuild.RuleFlags.Contains(TEXT("BaseCharacterId")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoV22PromotionRemovalMigrationTest,
                                 "ReEcho.Run.SaveV22PromotionRemovalMigration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoV22PromotionRemovalMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	Source->StartRun(TEXT("J_DIAMOND"), TEXT("W_J_01"));
	const FReEchoStatBlock OriginalStats = Source->CurrentBuild.Stats;
	UReEchoRunSubsystem* CurrentBrave = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	CurrentBrave->StartRun(TEXT("J_HEART"), TEXT("W_J_01"));
	const FReEchoStatBlock CurrentBraveStats = CurrentBrave->CurrentBuild.Stats;
	UReEchoRunSaveGame* LegacySave = Source->CreateSaveSnapshot();
	LegacySave->SaveVersion = 22;
	LegacySave->CurrentBuild.CharacterId = TEXT("J_HEART");
	LegacySave->CurrentBuild.RuleFlags.Add(TEXT("BaseCharacterId"), TEXT("J_DIAMOND"));
	LegacySave->CurrentBuild.RuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	LegacySave->CurrentBuild.RuleFlags.Add(TEXT("Role"), TEXT("Brave"));
	LegacySave->CurrentBuild.EquipmentBaseRuleFlags = LegacySave->CurrentBuild.RuleFlags;
	LegacySave->CurrentBuild.Stats.HpMax = 20.0f;
	LegacySave->CurrentBuild.Stats.ElementalAttack = 5.0f;
	LegacySave->CurrentBuild.Stats.MovementSpeed = 1.0f;
	LegacySave->CurrentBuild.Stats.CriticalRate = 0.2f;
	LegacySave->CurrentBuild.Stats.CriticalEffect = 0.5f;
	LegacySave->CurrentBuild.Stats.RoleId = TEXT("Brave");
	LegacySave->CurrentBuild.EquipmentBaseStats = LegacySave->CurrentBuild.Stats;

	// User accepted the existing current-catalog delta semantics on 2026-08-31.
	// Keep the historical payload: this characterizes today's migration, not historical-balance recovery.
	const float ExpectedHpMax = LegacySave->CurrentBuild.Stats.HpMax + OriginalStats.HpMax - CurrentBraveStats.HpMax;
	const float ExpectedElementalAttack = LegacySave->CurrentBuild.Stats.ElementalAttack +
	                                      OriginalStats.ElementalAttack - CurrentBraveStats.ElementalAttack;

	UGameInstance* RestoredGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Restored = NewObject<UReEchoRunSubsystem>(RestoredGameInstance);
	TestTrue(TEXT("v22 promoted save restores"), Restored->RestoreSaveSnapshot(*LegacySave));
	TestEqual(TEXT("Migration restores the selected Hunter identity"),
	          Restored->CurrentBuild.CharacterId,
	          FName(TEXT("J_DIAMOND")));
	TestEqual(TEXT("Migration restores the selected Hunter role"),
	          Restored->CurrentBuild.Stats.RoleId,
	          FName(TEXT("Hunter")));
	TestEqual(TEXT("Migration applies the current character-table health delta"),
	          Restored->CurrentBuild.Stats.HpMax,
	          ExpectedHpMax);
	TestEqual(TEXT("Migration applies the current character-table elemental-attack delta"),
	          Restored->CurrentBuild.Stats.ElementalAttack,
	          ExpectedElementalAttack);
	TestEqual(TEXT("Equipment base receives the same health delta"),
	          Restored->CurrentBuild.EquipmentBaseStats.HpMax,
	          ExpectedHpMax);
	TestEqual(TEXT("Equipment base receives the same elemental-attack delta"),
	          Restored->CurrentBuild.EquipmentBaseStats.ElementalAttack,
	          ExpectedElementalAttack);
	TestEqual(TEXT("Migration restores the Hunter movement ability"),
	          Restored->CurrentBuild.Stats.MovementSpeed,
	          OriginalStats.MovementSpeed);
	TestEqual(TEXT("Migration restores the Hunter critical-rate ability"),
	          Restored->CurrentBuild.Stats.CriticalRate,
	          OriginalStats.CriticalRate);
	TestFalse(TEXT("Migration consumes BaseCharacterId"),
	          Restored->CurrentBuild.RuleFlags.Contains(TEXT("BaseCharacterId")));
	TestFalse(TEXT("Migration consumes Promoted"), Restored->CurrentBuild.RuleFlags.Contains(TEXT("Promoted")));
	TestFalse(TEXT("Migration consumes Role"), Restored->CurrentBuild.RuleFlags.Contains(TEXT("Role")));

	UReEchoRunSaveGame* RoundTripSave = Restored->CreateSaveSnapshot();
	UGameInstance* RoundTripGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* RoundTrip = NewObject<UReEchoRunSubsystem>(RoundTripGameInstance);
	TestTrue(TEXT("Migrated save round-trips"), RoundTrip->RestoreSaveSnapshot(*RoundTripSave));
	TestEqual(
	    TEXT("Round-trip keeps the selected character"), RoundTrip->CurrentBuild.CharacterId, FName(TEXT("J_DIAMOND")));
	TestEqual(TEXT("Round-trip does not apply the health migration twice"),
	          RoundTrip->CurrentBuild.Stats.HpMax,
	          ExpectedHpMax);
	TestEqual(TEXT("Round-trip does not apply the elemental migration twice"),
	          RoundTrip->CurrentBuild.Stats.ElementalAttack,
	          ExpectedElementalAttack);

	UReEchoRunSaveGame* MissingOriginalSave = Source->CreateSaveSnapshot();
	MissingOriginalSave->SaveVersion = 22;
	MissingOriginalSave->CurrentBuild.CharacterId = TEXT("J_HEART");
	MissingOriginalSave->CurrentBuild.RuleFlags.Add(TEXT("BaseCharacterId"), TEXT("RETIRED_CHARACTER"));
	MissingOriginalSave->CurrentBuild.RuleFlags.Add(TEXT("Promoted"), TEXT("1"));
	MissingOriginalSave->CurrentBuild.Stats.RoleId = TEXT("Hunter");
	MissingOriginalSave->CurrentBuild.EquipmentBaseRuleFlags = MissingOriginalSave->CurrentBuild.RuleFlags;
	MissingOriginalSave->CurrentBuild.EquipmentBaseStats.RoleId = TEXT("Hunter");
	UGameInstance* FallbackGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Fallback = NewObject<UReEchoRunSubsystem>(FallbackGameInstance);
	TestTrue(TEXT("v22 save without a reliable original character restores"),
	         Fallback->RestoreSaveSnapshot(*MissingOriginalSave));
	TestEqual(TEXT("Missing original id keeps the current character instead of guessing"),
	          Fallback->CurrentBuild.CharacterId,
	          FName(TEXT("J_HEART")));
	TestEqual(TEXT("Missing original id normalizes role from the kept character"),
	          Fallback->CurrentBuild.Stats.RoleId,
	          FName(TEXT("Brave")));
	TestFalse(TEXT("Fallback migration also consumes legacy flags"),
	          Fallback->CurrentBuild.RuleFlags.Contains(TEXT("BaseCharacterId")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoV8CardMigrationTest,
                                 "ReEcho.Run.SaveV8CardIdsMigrateByMeaning",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoV8CardMigrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* SourceGameInstance = NewObject<UGameInstance>();
	UReEchoRunSubsystem* Source = NewObject<UReEchoRunSubsystem>(SourceGameInstance);
	Source->StartRun(TEXT("J_SPADE"), TEXT("W_J_01"));
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
