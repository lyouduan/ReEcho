#if WITH_DEV_AUTOMATION_TESTS

#include "ReEchoGameMode.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeFoxSpawnTest,
                                 "ReEcho.GameMode.GMSpawnFox",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoGameModeBossVictoryGateTest,
                                 "ReEcho.GameMode.BossVictoryRequiresSuccessfulSpawn",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

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
	const FVector2D ArenaCenter = FVector2D::ZeroVector;
	const FVector2D ArenaHalfExtents(1000.0f, 800.0f);
	const TArray<FVector> Locations = AReEchoGameMode::BuildGMSpawnFoxLocations(
	    PlayerLocation, ArenaCenter, ArenaHalfExtents, 25.0f, 5, 350.0f, true);
	TestEqual(TEXT("Batch layout returns one deterministic location per requested Fox"), Locations.Num(), 5);
	for (int32 Index = 0; Index < Locations.Num(); ++Index)
	{
		const FVector& Location = Locations[Index];
		TestTrue(TEXT("Fox batch location stays inside arena X bounds"), FMath::Abs(Location.X) <= ArenaHalfExtents.X);
		TestTrue(TEXT("Fox batch location stays inside arena Y bounds"), FMath::Abs(Location.Y) <= ArenaHalfExtents.Y);
		TestEqual(TEXT("Fox batch location uses the active gameplay plane"), Location.Z, 25.0);
		for (int32 EarlierIndex = 0; EarlierIndex < Index; ++EarlierIndex)
		{
			TestFalse(TEXT("Fox batch locations do not completely overlap"),
			          Locations[EarlierIndex].Equals(Location, KINDA_SMALL_NUMBER));
		}
	}

	const TArray<FVector> EdgeLocations = AReEchoGameMode::BuildGMSpawnFoxLocations(
	    FVector(-1000.0f, -800.0f, 75.0f), ArenaCenter, ArenaHalfExtents, 25.0f, 16, 1000.0f, true);
	TestEqual(TEXT("Maximum batch layout returns all requested Fox locations"), EdgeLocations.Num(), 16);
	for (int32 Index = 0; Index < EdgeLocations.Num(); ++Index)
	{
		for (int32 EarlierIndex = 0; EarlierIndex < Index; ++EarlierIndex)
		{
			TestFalse(TEXT("Arena-edge clamping does not collapse maximum-batch locations"),
			          EdgeLocations[EarlierIndex].Equals(EdgeLocations[Index], KINDA_SMALL_NUMBER));
		}
	}

	return true;
}

#endif
