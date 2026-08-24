#include "UI/ReEchoMinimapCanvasWidget.h"

#include "Rendering/DrawElements.h"
#include "SlateCore.h"
#include "Widgets/SCompoundWidget.h"

namespace
{
	/** 小地图固定像素尺寸。 */
	constexpr float MinimapCanvasSize = 220.0f;
}

void SReEchoMinimapCanvas::Construct(const FArguments& InArgs)
{
	View = FReEchoMinimapView();
	// 只绘制、不拦截鼠标，避免影响 game input。
	SetVisibility(EVisibility::HitTestInvisible);
}

FVector2D SReEchoMinimapCanvas::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(MinimapCanvasSize, MinimapCanvasSize);
}

FVector2D SReEchoMinimapCanvas::ToLocal(const FVector2D& WorldXY, const FVector2D& CanvasSize) const
{
	return SReEchoMinimapCanvas::TransformWorldToMinimap(WorldXY, View.ArenaCenter, View.ArenaHalfExtents, CanvasSize);
}

FVector2D SReEchoMinimapCanvas::TransformWorldToMinimap(const FVector2D& WorldXY,
                                                       const FVector2D& Center,
                                                       const FVector2D& HalfExtents,
                                                       const FVector2D& CanvasSize)
{
	// 以竞技场中心为中点，归一化到 [0,1]。
	const FVector2D Min = Center - HalfExtents;
	const FVector2D Norm = (WorldXY - Min) / (2.0 * HalfExtents);
	const double ClampedX = FMath::Clamp(Norm.X, 0.0, 1.0);
	const double ClampedY = FMath::Clamp(Norm.Y, 0.0, 1.0);
	// 对齐实际 3D 视角：竞技场相机位于 -X 侧、俯视 45°（FRotator(-45,0,0)），
	// 屏幕右=世界 +Y、屏幕上=世界 +X。故 右轴取世界 Y；纵向以世界 +X 朝上 => 翻转 X。
	return FVector2D(ClampedY * CanvasSize.X, (1.0 - ClampedX) * CanvasSize.Y);
}

int32 SReEchoMinimapCanvas::OnPaint(const FPaintArgs& Args,
                                    const FGeometry& AllottedGeometry,
                                    const FSlateRect& MyCullingRect,
                                    FSlateWindowElementList& OutDrawElements,
                                    int32 LayerId,
                                    const FWidgetStyle& InWidgetStyle,
                                    bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	if (Size.X <= 1.0 || Size.Y <= 1.0)
	{
		return LayerId;
	}

	// 白色实心刷仅用于轨迹点和玩家点；画布本身保持透明，
	// 让 WBP 中的正式回响边框素材决定底板视觉。
	FSlateBrush SolidBrush;
	SolidBrush.TintColor = FSlateColor(FLinearColor::White);

	if (View.bValid && View.ArenaHalfExtents.X > KINDA_SMALL_NUMBER && View.ArenaHalfExtents.Y > KINDA_SMALL_NUMBER)
	{
		// 各回响轨迹折线 + 当前点
		for (const FReEchoMinimapEchoEntry& Entry : View.Echoes)
		{
			TArray<FVector2D> Points;
			Points.Reserve(Entry.PathPoints.Num());
			for (const FVector2D& WorldXY : Entry.PathPoints)
			{
				Points.Add(ToLocal(WorldXY, Size));
			}
			if (Points.Num() >= 2)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId + 2,
					AllottedGeometry.ToPaintGeometry(),
					Points,
					ESlateDrawEffect::None,
					Entry.Color,
					true,
					2.0f);
			}
			const FVector2D EchoPos = ToLocal(Entry.CurrentLocation, Size);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 3,
				AllottedGeometry.ToPaintGeometry(FVector2f(8.0f, 8.0f), FSlateLayoutTransform(1.0f, FVector2f(static_cast<float>(EchoPos.X - 4.0), static_cast<float>(EchoPos.Y - 4.0)))),
				&SolidBrush,
				ESlateDrawEffect::None,
				Entry.Color);
		}

		// 玩家当前点（高亮绿）
		const FVector2D PlayerPos = ToLocal(View.PlayerLocation, Size);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId + 4,
			AllottedGeometry.ToPaintGeometry(FVector2f(12.0f, 12.0f), FSlateLayoutTransform(1.0f, FVector2f(static_cast<float>(PlayerPos.X - 6.0), static_cast<float>(PlayerPos.Y - 6.0)))),
			&SolidBrush,
			ESlateDrawEffect::None,
			FLinearColor(0.2f, 1.0f, 0.4f, 1.0f));
	}

	return LayerId + 5;
}

TSharedRef<SWidget> UReEchoMinimapCanvasWidget::RebuildWidget()
{
	Canvas = SNew(SReEchoMinimapCanvas);
	return Canvas.ToSharedRef();
}

void UReEchoMinimapCanvasWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Canvas.Reset();
}

void UReEchoMinimapCanvasWidget::SetView(const FReEchoMinimapView& InView)
{
	if (Canvas.IsValid())
	{
		Canvas->SetView(InView);
	}
}

FVector2D UReEchoMinimapCanvasWidget::TransformWorldToMinimap(const FVector2D& WorldXY,
                                                              const FVector2D& Center,
                                                              const FVector2D& HalfExtents,
                                                              const FVector2D& CanvasSize)
{
	return SReEchoMinimapCanvas::TransformWorldToMinimap(WorldXY, Center, HalfExtents, CanvasSize);
}
