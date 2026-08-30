#if WITH_DEV_AUTOMATION_TESTS

#include "ReEchoGameMode.h"

#include "Components/LineBatchComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Run/ReEchoRunSubsystem.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeFoxSpawnTest,
                                 "ReEcho.GameMode.GMSpawnFox",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeBossVictoryGateTest,
                                 "ReEcho.GameMode.BossVictoryRequiresSuccessfulSpawn",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeSceneAndMoveSpeedTest,
                                 "ReEcho.GameMode.GMSceneAndMoveSpeed",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeEnemyElementAllTest,
                                 "ReEcho.GameMode.GMEnemyElementAll",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeNewGameSaveSlotTest,
                                 "ReEcho.GameMode.NewGameSaveSlot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeSpawnWarningNoDebugGeometryTest,
                                 "ReEcho.GameMode.SpawnWarningNoDebugGeometry",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoGameModeSpawnWarningNoDebugGeometryTest::RunTest(const FString& Parameters)
{
	const FReEchoCsvLoadResult Load =
	    FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(FReEchoCsvDataRegistry::GetDefaultDataDirectory());
	if (!TestTrue(TEXT("Production data loads"), Load.bSuccess) || !Load.Snapshot.IsValid())
	{
		AddError(Load.FormatIssues());
		return false;
	}
	for (const EWorldType::Type WorldType : {EWorldType::Editor, EWorldType::Game})
	{
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("SpawnWarningTestWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(WorldType);
		UWorld* World = UWorld::CreateWorld(WorldType, false, WorldName, GetTransientPackage());
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		ON_SCOPE_EXIT
		{
			World->DestroyWorld(true);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		};
		AReEchoGameMode* GameMode = World->SpawnActor<AReEchoGameMode>();
		if (!TestNotNull(TEXT("Spawn warning host exists"), GameMode))
		{
			return false;
		}
		for (const FName EncounterId : {FName(TEXT("Encounter.1")), FName(TEXT("Encounter.8"))})
		{
			FString Error;
			FReEchoEncounterWaveScheduler Probe;
			if (!TestTrue(TEXT("Probe schedule compiles"), Probe.Configure(*Load.Snapshot, EncounterId, Error)) ||
			    !TestTrue(TEXT("Host schedule compiles"),
			              GameMode->EncounterWaveScheduler.Configure(*Load.Snapshot, EncounterId, Error)))
			{
				return false;
			}
			const TArray<FReEchoScheduledSpawnEvent> Warnings = Probe.AdvanceTo(0.0f);
			TestTrue(TEXT("Initial wave has warning events"), Warnings.Num() > 0);
			GameMode->PendingSpawnBatches.Reset();
			// Seed resolved reservations so this test isolates the warning consumer from world placement.
			const FVector ReservedLocation(125.0f, 250.0f, 215.0f);
			for (const FReEchoScheduledSpawnEvent& Event : Warnings)
			{
				if (!TestEqual(
				        TEXT("Initial events are warnings"), Event.Type, EReEchoScheduledSpawnEventType::Warning))
				{
					return false;
				}
				FReEchoPendingSpawnBatchState& Pending = GameMode->PendingSpawnBatches.AddDefaulted_GetRef();
				Pending.WaveId = Event.WaveId;
				Pending.EnemyRole = Event.EnemyRole;
				Pending.EnemyId = Event.EnemyId;
				Pending.Locations.Add(ReservedLocation);
			}
			GameMode->ProcessScheduledSpawnEvents(0.0f);
			TestEqual(TEXT("Warnings still advance the authoritative cursor"),
			          GameMode->EncounterWaveScheduler.GetNextEventIndex(),
			          Probe.GetNextEventIndex());
			TestEqual(TEXT("All reservations survive warning processing"),
			          GameMode->PendingSpawnBatches.Num(),
			          Warnings.Num());
			for (const FReEchoPendingSpawnBatchState& Pending : GameMode->PendingSpawnBatches)
			{
				TestTrue(TEXT("Reserved locations are unchanged"),
				         Pending.Locations == TArray<FVector>{ReservedLocation});
			}
			for (const UWorld::ELineBatcherType BatcherType : {UWorld::ELineBatcherType::World,
			                                                   UWorld::ELineBatcherType::WorldPersistent,
			                                                   UWorld::ELineBatcherType::Foreground})
			{
				const ULineBatchComponent* Batcher = World->GetLineBatcher(BatcherType);
				if (TestNotNull(TEXT("World has a testable debug line batcher"), Batcher))
				{
					TestEqual(TEXT("Spawn warnings do not draw debug lines"), Batcher->BatchedLines.Num(), 0);
					TestEqual(TEXT("Spawn warnings do not draw debug meshes"), Batcher->BatchedMeshes.Num(), 0);
					TestEqual(TEXT("Spawn warnings do not draw debug points"), Batcher->BatchedPoints.Num(), 0);
				}
			}
		}
	}
	return true;
}

bool FReEchoGameModeNewGameSaveSlotTest::RunTest(const FString& Parameters)
{
	TArray<FReEchoSaveSlotSummary> Slots;
	for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
	{
		FReEchoSaveSlotSummary& Slot = Slots.AddDefaulted_GetRef();
		Slot.SlotIndex = SlotIndex;
		Slot.bOccupied = true;
		Slot.SavedAtUtc = FDateTime(2026, 8, 30, 10 + SlotIndex, 0, 0);
	}
	Slots[1].bOccupied = false;
	TestEqual(
	    TEXT("New game prefers the first physical empty slot"), AReEchoGameMode::ResolveNewGameSaveSlot(Slots), 1);

	Slots[1].bOccupied = true;
	Slots[1].SavedAtUtc = FDateTime(2026, 8, 29, 8, 0, 0);
	TestEqual(TEXT("A full save set selects the oldest slot without deleting it"),
	          AReEchoGameMode::ResolveNewGameSaveSlot(Slots),
	          1);

	Slots[0].SavedAtUtc = Slots[1].SavedAtUtc;
	TestEqual(TEXT("Equal save times choose the lowest stable slot index"),
	          AReEchoGameMode::ResolveNewGameSaveSlot(Slots),
	          0);
	TestEqual(TEXT("An empty summary rejects new game allocation"),
	          AReEchoGameMode::ResolveNewGameSaveSlot(TArray<FReEchoSaveSlotSummary>()),
	          INDEX_NONE);
	return true;
}

bool FReEchoGameModeSceneAndMoveSpeedTest::RunTest(const FString& Parameters)
{
	FName SceneId = NAME_None;
	TestTrue(TEXT("Canonical scene id is accepted"), AReEchoGameMode::TryResolveGMSceneId(TEXT("SC02"), SceneId));
	TestEqual(TEXT("Canonical scene id is preserved"), SceneId, FName(TEXT("SC02")));
	TestTrue(TEXT("Short scene number is normalized"), AReEchoGameMode::TryResolveGMSceneId(TEXT("3"), SceneId));
	TestEqual(TEXT("Short scene number resolves to SC03"), SceneId, FName(TEXT("SC03")));
	TestFalse(TEXT("Unknown scene is rejected"), AReEchoGameMode::TryResolveGMSceneId(TEXT("SC05"), SceneId));
	TestFalse(TEXT("Non-numeric scene is rejected"), AReEchoGameMode::TryResolveGMSceneId(TEXT("Forest"), SceneId));
	TestTrue(TEXT("Positive movement speed is accepted"), AReEchoGameMode::IsValidGMMoveSpeed(600.0f));
	TestFalse(TEXT("Zero movement speed is rejected"), AReEchoGameMode::IsValidGMMoveSpeed(0.0f));
	TestFalse(TEXT("Negative movement speed is rejected"), AReEchoGameMode::IsValidGMMoveSpeed(-1.0f));
	TestFalse(TEXT("Non-finite movement speed is rejected"),
	          AReEchoGameMode::IsValidGMMoveSpeed(std::numeric_limits<float>::infinity()));
	return true;
}

bool FReEchoGameModeEnemyElementAllTest::RunTest(const FString& Parameters)
{
	EReEchoElement Element = EReEchoElement::Flame;
	TestTrue(TEXT("GM all-enemy attachment accepts Grass"),
	         AReEchoGameMode::TryResolveGMEnemyAttachment(TEXT("grass"), Element));
	TestEqual(TEXT("GM all-enemy attachment resolves Grass"), Element, EReEchoElement::Grass);
	TestTrue(TEXT("GM all-enemy attachment accepts Water"),
	         AReEchoGameMode::TryResolveGMEnemyAttachment(TEXT("Water"), Element));
	TestEqual(TEXT("GM all-enemy attachment resolves Water"), Element, EReEchoElement::Water);
	TestTrue(TEXT("GM all-enemy attachment accepts Clear"),
	         AReEchoGameMode::TryResolveGMEnemyAttachment(TEXT("Clear"), Element));
	TestEqual(TEXT("GM all-enemy attachment resolves Clear to None"), Element, EReEchoElement::None);
	TestFalse(TEXT("GM all-enemy attachment rejects trigger-only Flame"),
	          AReEchoGameMode::TryResolveGMEnemyAttachment(TEXT("Flame"), Element));
	TestFalse(TEXT("GM all-enemy attachment rejects trigger-only Lightning"),
	          AReEchoGameMode::TryResolveGMEnemyAttachment(TEXT("Lightning"), Element));
	return true;
}

bool FReEchoGameModeBossVictoryGateTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("A missing or failed Boss spawn cannot be interpreted as victory"),
	          AReEchoGameMode::ShouldCompleteBossEncounter(false, 0));
	TestFalse(TEXT("A living successfully spawned Boss keeps the encounter running"),
	          AReEchoGameMode::ShouldCompleteBossEncounter(true, 1));
	TestTrue(TEXT("Defeating a successfully spawned Boss allows victory"),
	         AReEchoGameMode::ShouldCompleteBossEncounter(true, 0));
	return true;
}

bool FReEchoGameModeFoxSpawnTest::RunTest(const FString& Parameters)
{
	int32 Count = 0;
	float Distance = 0.0f;
	bool bLegacyDistance = false;
	AReEchoGameMode::ResolveGMSpawnFoxRequest(1.0f, -1.0f, Count, Distance, bLegacyDistance);
	TestEqual(TEXT("No-argument defaults preserve one Fox"), Count, 1);
	TestEqual(TEXT("No-argument defaults preserve the safe distance"), Distance, 350.0f);
	TestFalse(TEXT("No-argument defaults are not reported as legacy syntax"), bLegacyDistance);

	AReEchoGameMode::ResolveGMSpawnFoxRequest(475.0f, -1.0f, Count, Distance, bLegacyDistance);
	TestEqual(TEXT("Legacy single-distance syntax preserves one Fox"), Count, 1);
	TestEqual(TEXT("Legacy single-distance syntax preserves its distance"), Distance, 475.0f);
	TestTrue(TEXT("Legacy single-distance syntax is named in the result"), bLegacyDistance);

	AReEchoGameMode::ResolveGMSpawnFoxRequest(5.0f, 350.0f, Count, Distance, bLegacyDistance);
	TestEqual(TEXT("Two-argument syntax resolves the requested Fox count"), Count, 5);
	TestEqual(TEXT("Two-argument syntax resolves the requested Fox distance"), Distance, 350.0f);
	TestFalse(TEXT("Two-argument syntax is not legacy"), bLegacyDistance);

	AReEchoGameMode::ResolveGMSpawnFoxRequest(999.0f, 5000.0f, Count, Distance, bLegacyDistance);
	TestEqual(TEXT("Fox batch count is clamped to its safety ceiling"), Count, 16);
	TestEqual(TEXT("Fox spawn distance is clamped to its safety ceiling"), Distance, 1000.0f);

	const FVector PlayerLocation(-800.0f, 0.0f, 75.0f);
	const FBox2D SpawnWorldBounds(FVector2D(-700.0f, -1000.0f), FVector2D(1300.0f, 600.0f));
	const TArray<FVector> Locations = AReEchoGameMode::BuildGMSpawnFoxLocations(
	    PlayerLocation, SpawnWorldBounds, 25.0f, 5, 350.0f, true);
	TestEqual(TEXT("Batch layout returns one deterministic location per requested Fox"), Locations.Num(), 5);
	for (int32 Index = 0; Index < Locations.Num(); ++Index)
	{
		const FVector& Location = Locations[Index];
		TestTrue(TEXT("Fox batch location stays inside translated wall-derived bounds"),
		         SpawnWorldBounds.IsInside(FVector2D(Location.X, Location.Y)));
		TestEqual(TEXT("Fox batch location uses the active gameplay plane"), Location.Z, 25.0);
		for (int32 EarlierIndex = 0; EarlierIndex < Index; ++EarlierIndex)
		{
			TestFalse(TEXT("Fox batch locations do not completely overlap"),
			          Locations[EarlierIndex].Equals(Location, KINDA_SMALL_NUMBER));
		}
	}

	const TArray<FVector> EdgeLocations = AReEchoGameMode::BuildGMSpawnFoxLocations(
	    FVector(-700.0f, -1000.0f, 75.0f), SpawnWorldBounds, 25.0f, 16, 1000.0f, true);
	TestEqual(TEXT("Maximum batch layout returns all requested Fox locations"), EdgeLocations.Num(), 16);
	for (int32 Index = 0; Index < EdgeLocations.Num(); ++Index)
	{
		TestTrue(TEXT("Maximum batch fallback remains inside wall-derived bounds"),
		         SpawnWorldBounds.IsInside(FVector2D(EdgeLocations[Index].X, EdgeLocations[Index].Y)));
		for (int32 EarlierIndex = 0; EarlierIndex < Index; ++EarlierIndex)
		{
			TestFalse(TEXT("Arena-edge clamping does not collapse maximum-batch locations"),
			          EdgeLocations[EarlierIndex].Equals(EdgeLocations[Index], KINDA_SMALL_NUMBER));
		}
	}

	return true;
}

#endif
