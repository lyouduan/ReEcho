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

#endif // WITH_DEV_AUTOMATION_TESTS
