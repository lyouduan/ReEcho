#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/ReEchoMinimapCanvasWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoMinimapTransformTest,
                                 "ReEcho.UI.Minimap.Transform",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoMinimapTransformTest::RunTest(const FString& Parameters)
{
	const FVector2D Center(0.0, 0.0);
	const FVector2D Half(100.0, 200.0);
	const FVector2D Size(200.0, 400.0);

	// 小地图对齐实际 3D 视角：屏幕右=世界+Y、屏幕上=世界+X。
	// 因此 右轴取世界 Y，纵向以世界 +X 朝上（翻转 X）。

	// Arena center maps to canvas center.
	const FVector2D CenterPt = UReEchoMinimapCanvasWidget::TransformWorldToMinimap(Center, Center, Half, Size);
	TestTrue(TEXT("Center maps to canvas center X"), FMath::IsNearlyEqual(CenterPt.X, 100.0));
	TestTrue(TEXT("Center maps to canvas center Y"), FMath::IsNearlyEqual(CenterPt.Y, 200.0));

	// 世界 (-X,-Y)：世界-Y=左，世界-X=下 => 画布左下 (0, Size.Y)。
	const FVector2D BL = UReEchoMinimapCanvasWidget::TransformWorldToMinimap(Center - Half, Center, Half, Size);
	TestTrue(TEXT("World (-X,-Y) maps to canvas (0,Size.Y) X"), FMath::IsNearlyEqual(BL.X, 0.0));
	TestTrue(TEXT("World (-X,-Y) maps to canvas (0,Size.Y) Y"), FMath::IsNearlyEqual(BL.Y, 400.0));

	// 世界 (+X,+Y)：世界+Y=右，世界+X=上 => 画布右上 (Size.X, 0)。
	const FVector2D TR = UReEchoMinimapCanvasWidget::TransformWorldToMinimap(Center + Half, Center, Half, Size);
	TestTrue(TEXT("World (+X,+Y) maps to canvas (Size.X,0) X"), FMath::IsNearlyEqual(TR.X, 200.0));
	TestTrue(TEXT("World (+X,+Y) maps to canvas (Size.X,0) Y"), FMath::IsNearlyEqual(TR.Y, 0.0));

	// 世界 (+X,-Y)：世界-Y=左，世界+X=上 => 画布左上 (0, 0)。
	const FVector2D TL = UReEchoMinimapCanvasWidget::TransformWorldToMinimap(
	    FVector2D(Center.X + Half.X, Center.Y - Half.Y), Center, Half, Size);
	TestTrue(TEXT("World (+X,-Y) maps to (0,0) X"), FMath::IsNearlyEqual(TL.X, 0.0));
	TestTrue(TEXT("World (+X,-Y) maps to (0,0) Y"), FMath::IsNearlyEqual(TL.Y, 0.0));

	// 越界 clamp 到 [0,Size]。世界 Y 越界 => 小地图 X 贴右。
	const FVector2D Out = UReEchoMinimapCanvasWidget::TransformWorldToMinimap(
	    FVector2D(Center.X, Center.Y + 3.0 * Half.Y), Center, Half, Size);
	TestTrue(TEXT("Out-of-bounds Y clamps canvas X to Size"), FMath::IsNearlyEqual(Out.X, 200.0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoMinimapInkTrailSamplingTest,
                                 "ReEcho.UI.Minimap.InkTrailSampling",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoMinimapInkTrailSamplingTest::RunTest(const FString& Parameters)
{
	const TArray<FVector2D> Straight = {FVector2D(0.0, 0.0), FVector2D(10.0, 0.0)};
	const TArray<FReEchoMinimapInkStamp> StraightStamps =
	    SReEchoMinimapCanvas::BuildInkTrailStamps(Straight, 2.5f, 0.0f, 0.0f, 17, 64);
	TestEqual(TEXT("Straight trail uses equal-distance stamps including both endpoints"), StraightStamps.Num(), 5);
	for (int32 Index = 0; Index < StraightStamps.Num(); ++Index)
	{
		TestTrue(TEXT("Straight stamp position is equally spaced"),
		         StraightStamps[Index].Position.Equals(FVector2D(Index * 2.5, 0.0), UE_KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Straight stamp follows segment direction"),
		         FMath::IsNearlyZero(StraightStamps[Index].AngleRadians));
		TestTrue(TEXT("Zero opacity jitter remains opaque"), FMath::IsNearlyEqual(StraightStamps[Index].Opacity, 1.0f));
	}

	const TArray<FVector2D> Corner = {FVector2D(0.0, 0.0), FVector2D(3.0, 0.0), FVector2D(3.0, 4.0)};
	const TArray<FReEchoMinimapInkStamp> CornerStamps =
	    SReEchoMinimapCanvas::BuildInkTrailStamps(Corner, 2.0f, 0.0f, 0.0f, 17, 64);
	TestEqual(TEXT("Spacing carries across segment boundaries"), CornerStamps.Num(), 4);
	if (CornerStamps.Num() == 4)
	{
		TestTrue(TEXT("First stamp after corner preserves arc spacing"),
		         CornerStamps[2].Position.Equals(FVector2D(3.0, 1.0), UE_KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Second segment stamp follows vertical direction"),
		         FMath::IsNearlyEqual(CornerStamps[2].AngleRadians, PI * 0.5f));
	}

	const TArray<FReEchoMinimapInkStamp> JitteredA =
	    SReEchoMinimapCanvas::BuildInkTrailStamps(Straight, 1.0f, 18.0f, 0.16f, 1337, 64);
	const TArray<FReEchoMinimapInkStamp> JitteredB =
	    SReEchoMinimapCanvas::BuildInkTrailStamps(Straight, 1.0f, 18.0f, 0.16f, 1337, 64);
	TestEqual(TEXT("Deterministic trail uses a stable stamp count"), JitteredA.Num(), JitteredB.Num());
	for (int32 Index = 0; Index < FMath::Min(JitteredA.Num(), JitteredB.Num()); ++Index)
	{
		TestTrue(TEXT("Deterministic trail keeps the same position"),
		         JitteredA[Index].Position.Equals(JitteredB[Index].Position, UE_KINDA_SMALL_NUMBER));
		TestTrue(TEXT("Deterministic trail keeps the same angle"),
		         FMath::IsNearlyEqual(JitteredA[Index].AngleRadians, JitteredB[Index].AngleRadians));
		TestTrue(TEXT("Deterministic trail keeps the same opacity"),
		         FMath::IsNearlyEqual(JitteredA[Index].Opacity, JitteredB[Index].Opacity));
		TestTrue(TEXT("Opacity jitter remains in the valid range"),
		         JitteredA[Index].Opacity >= 0.84f && JitteredA[Index].Opacity <= 1.0f);
	}

	const TArray<FVector2D> Degenerate = {FVector2D(2.0, 2.0), FVector2D(2.0, 2.0), FVector2D(6.0, 2.0)};
	const TArray<FReEchoMinimapInkStamp> DegenerateStamps =
	    SReEchoMinimapCanvas::BuildInkTrailStamps(Degenerate, 0.0f, 500.0f, 5.0f, 7, 3);
	TestEqual(TEXT("Degenerate and dense trails obey the hard stamp limit"), DegenerateStamps.Num(), 3);
	for (const FReEchoMinimapInkStamp& Stamp : DegenerateStamps)
	{
		TestTrue(TEXT("Degenerate trail never produces NaN position"), !Stamp.Position.ContainsNaN());
		TestTrue(TEXT("Degenerate trail never produces NaN angle"), FMath::IsFinite(Stamp.AngleRadians));
		TestTrue(TEXT("Clamped opacity jitter remains valid"), Stamp.Opacity >= 0.0f && Stamp.Opacity <= 1.0f);
	}

	const FLinearColor FallbackColor(0.25f, 0.5f, 0.75f, 1.0f);
	const TArray<FLinearColor> BlueprintColors = {FLinearColor::Red, FLinearColor::Green};
	TestEqual(TEXT("Blueprint trail palette overrides the matching Echo color"),
	          SReEchoMinimapCanvas::ResolveInkTrailColor(BlueprintColors, 1, FallbackColor),
	          FLinearColor::Green);
	TestEqual(TEXT("Missing Blueprint trail palette entries preserve the runtime fallback"),
	          SReEchoMinimapCanvas::ResolveInkTrailColor(BlueprintColors, 4, FallbackColor),
	          FallbackColor);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
