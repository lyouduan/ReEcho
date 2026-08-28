#include "Presentation/Scene/ReEchoArenaSceneActor.h"
#include "Presentation/Scene/ReEchoArenaSceneProfile.h"

#include "Components/BoxComponent.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoArenaSceneContractTest,
                                 "ReEcho.Presentation.ArenaScene.Contract",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoArenaSceneContractTest::RunTest(const FString& Parameters)
{
	const UReEchoArenaSceneProfile* DefaultProfile = GetDefault<UReEchoArenaSceneProfile>();
	TestEqual(TEXT("Scene profile defaults keep a broad center safe zone"),
	          DefaultProfile->CenterSafeZoneRatio,
	          FVector2D(0.6f, 0.6f));
	TestEqual(TEXT("Scene profile defaults permit at most one landmark"), DefaultProfile->MaximumLandmarks, 1);

	const FVector2D Footprint = AReEchoArenaSceneActor::CalculateGroundFootprintHalfExtents(
	    2800.0f, 1376.0f / 768.0f, FRotator(-45.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Projected footprint has positive X extent"), Footprint.X > 0.0f);
	TestTrue(TEXT("Projected footprint has positive Y extent"), Footprint.Y > 0.0f);

	const FVector2D Center(200.0f, -300.0f);
	const FVector2D MapHalfExtents(2500.0f, 3000.0f);
	const FVector2D DesiredInside(400.0f, -100.0f);
	TestEqual(TEXT("Safe-center focus follows the player"),
	          AReEchoArenaSceneActor::ClampCameraFocus(DesiredInside, Center, MapHalfExtents, Footprint),
	          DesiredInside);

	const FVector2D SafeHalfExtents = MapHalfExtents - Footprint;
	const FVector2D PositiveCorner =
	    AReEchoArenaSceneActor::ClampCameraFocus(FVector2D(100000.0f, 100000.0f), Center, MapHalfExtents, Footprint);
	TestEqual(TEXT("Positive X edge clamps"), PositiveCorner.X, Center.X + SafeHalfExtents.X);
	TestEqual(TEXT("Positive Y edge clamps"), PositiveCorner.Y, Center.Y + SafeHalfExtents.Y);
	const FVector2D NegativeCorner =
	    AReEchoArenaSceneActor::ClampCameraFocus(FVector2D(-100000.0f, -100000.0f), Center, MapHalfExtents, Footprint);
	TestEqual(TEXT("Negative X edge clamps"), NegativeCorner.X, Center.X - SafeHalfExtents.X);
	TestEqual(TEXT("Negative Y edge clamps"), NegativeCorner.Y, Center.Y - SafeHalfExtents.Y);

	const FVector2D UndersizedResult =
	    AReEchoArenaSceneActor::ClampCameraFocus(FVector2D(100000.0f, -100000.0f), Center, Footprint * 0.5f, Footprint);
	TestEqual(TEXT("Undersized map locks both axes to center"), UndersizedResult, Center);

	const FVector2D AsymmetricResult =
	    AReEchoArenaSceneActor::ClampCameraFocusWithInsets(FVector2D(100000.0f, -100000.0f),
	                                                       Center,
	                                                       MapHalfExtents,
	                                                       FVector2D(150.0f, 250.0f),
	                                                       FVector2D(350.0f, 450.0f),
	                                                       Footprint);
	TestEqual(TEXT("Positive X edge applies its independent inset"),
	          AsymmetricResult.X,
	          Center.X + MapHalfExtents.X - 350.0f - Footprint.X);
	TestEqual(TEXT("Negative Y edge applies its independent inset"),
	          AsymmetricResult.Y,
	          Center.Y - MapHalfExtents.Y + 250.0f + Footprint.Y);

	const FIntPoint SortRange(-10, 10);
	const int32 NearPriority = AReEchoArenaSceneActor::CalculateFootpointSortPriority(
	    FVector2D(100.0f, 0.0f), FVector2D::ZeroVector, FVector2D(1.0f, 0.0f), 10.0f, 100, SortRange);
	const int32 FarPriority = AReEchoArenaSceneActor::CalculateFootpointSortPriority(
	    FVector2D(50.0f, 0.0f), FVector2D::ZeroVector, FVector2D(1.0f, 0.0f), 10.0f, 100, SortRange);
	TestTrue(TEXT("Footpoint sorting is monotonic along its configured axis"), NearPriority > FarPriority);
	TestEqual(TEXT("Footpoint sorting clamps to its configured range"), NearPriority, 110);

	const FTransform ShiftedMap(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 137.0f), FVector(1.0f, 1.0f, 2.0f));
	TestEqual(TEXT("MapRoot transform moves and scales the gameplay plane"),
	          AReEchoArenaSceneActor::CalculateGameplayPlaneWorldZ(ShiftedMap, 5.0f),
	          147.0f);

	const FBox PlaneBounds(FVector(-50.0f, -50.0f, 0.0f), FVector(50.0f, 50.0f, 0.0f));
	const FTransform RotatedBackdrop(FRotator(0.0f, 90.0f, 0.0f),
	                                 FVector(100.0f, 200.0f, -0.5f),
	                                 FVector(44.8f, 25.0f, 1.0f));
	const FBox2D RotatedFootprint =
	    AReEchoArenaSceneActor::CalculateWorldXYBounds(PlaneBounds, RotatedBackdrop);
	TestTrue(TEXT("Rotated Backdrop mesh bounds remain valid"), RotatedFootprint.bIsValid);
	TestEqual(TEXT("Backdrop rotation maps mesh Y scale onto world X"), RotatedFootprint.GetSize().X, 2500.0);
	TestEqual(TEXT("Backdrop rotation maps mesh X scale onto world Y"), RotatedFootprint.GetSize().Y, 4480.0);

	FBox2D SpawnBounds(ForceInit);
	FString SpawnBoundsError;
	TestTrue(TEXT("Four asymmetric translated wall AABBs form a spawn-safe rectangle"),
	         AReEchoArenaSceneActor::CalculateWallDerivedSpawnBounds(
	             FBox2D(FVector2D(75.0f, -1000.0f), FVector2D(125.0f, 3000.0f)),
	             FBox2D(FVector2D(2075.0f, -1000.0f), FVector2D(2125.0f, 3000.0f)),
	             FBox2D(FVector2D(0.0f, -2025.0f), FVector2D(2200.0f, -1975.0f)),
	             FBox2D(FVector2D(0.0f, 2975.0f), FVector2D(2200.0f, 3025.0f)),
	             100.0f,
	             SpawnBounds,
	             &SpawnBoundsError));
	TestEqual(TEXT("West inner face and padding define minimum X"), SpawnBounds.Min.X, 225.0);
	TestEqual(TEXT("East inner face and padding define maximum X"), SpawnBounds.Max.X, 1975.0);
	TestEqual(TEXT("South inner face and padding define minimum Y"), SpawnBounds.Min.Y, -1875.0);
	TestEqual(TEXT("North inner face and padding define maximum Y"), SpawnBounds.Max.Y, 2875.0);
	TestTrue(TEXT("Mirrored names and ninety-degree role rotation still derive the geometric enclosure"),
	         AReEchoArenaSceneActor::CalculateWallDerivedSpawnBounds(
	             // A/B are geometrically top/bottom despite occupying the former West/East argument slots.
	             FBox2D(FVector2D(0.0f, 2975.0f), FVector2D(2200.0f, 3025.0f)),
	             FBox2D(FVector2D(0.0f, -2025.0f), FVector2D(2200.0f, -1975.0f)),
	             // C/D are geometrically right/left and are intentionally reversed.
	             FBox2D(FVector2D(2075.0f, -1000.0f), FVector2D(2125.0f, 3000.0f)),
	             FBox2D(FVector2D(75.0f, -1000.0f), FVector2D(125.0f, 3000.0f)),
	             100.0f,
	             SpawnBounds,
	             &SpawnBoundsError));
	TestEqual(TEXT("Mirrored wall assignment preserves minimum corner"), SpawnBounds.Min, FVector2D(225.0f, -1875.0f));
	TestEqual(TEXT("Mirrored wall assignment preserves maximum corner"), SpawnBounds.Max, FVector2D(1975.0f, 2875.0f));
	TestFalse(TEXT("A genuinely too-narrow enclosure still fails closed after padding"),
	          AReEchoArenaSceneActor::CalculateWallDerivedSpawnBounds(
	              FBox2D(FVector2D(0.0f, -500.0f), FVector2D(50.0f, 500.0f)),
	              FBox2D(FVector2D(200.0f, -500.0f), FVector2D(250.0f, 500.0f)),
	              FBox2D(FVector2D(0.0f, -550.0f), FVector2D(250.0f, -500.0f)),
	              FBox2D(FVector2D(0.0f, 500.0f), FVector2D(250.0f, 550.0f)),
	              100.0f,
	              SpawnBounds,
	              &SpawnBoundsError));
	TestTrue(TEXT("Degenerate diagnostic includes the computed padded bounds"),
	         SpawnBoundsError.Contains(TEXT("padded bounds")));

	AReEchoArenaSceneActor* Arena = NewObject<AReEchoArenaSceneActor>(GetTransientPackage());
	Arena->SetActorTransform(ShiftedMap);
	Arena->MapRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 500.0f));
	Arena->MapRoot->SetRelativeScale3D(FVector(1.0f, 1.0f, 7.0f));
	Arena->GameplayPlaneZ = 5.0f;
	TestEqual(TEXT("Map visual transform cannot change gameplay plane world Z"),
	          Arena->GetGameplayPlaneWorldZ(),
	          147.0f);
	Arena->PlayerBounds->SetBoxExtent(FVector(1234.0f, 2345.0f, 5.0f));
	Arena->CameraClampBounds->SetBoxExtent(FVector(1334.0f, 2445.0f, 5.0f));
	Arena->EnemySpawnBounds->SetBoxExtent(FVector(1134.0f, 2245.0f, 5.0f));
	TestEqual(TEXT("Player bounds consume the authored BoxComponent extent"),
	          Arena->GetPlayerHalfExtents(),
	          FVector2D(1234.0f, 2345.0f));
	TestEqual(TEXT("Camera bounds consume the authored BoxComponent extent"),
	          Arena->GetCameraClampHalfExtents(),
	          FVector2D(1334.0f, 2445.0f));
	TestEqual(TEXT("Enemy spawn bounds consume the authored BoxComponent extent"),
	          Arena->GetEnemySpawnHalfExtents(),
	          FVector2D(1134.0f, 2245.0f));
	return true;
}

#endif
