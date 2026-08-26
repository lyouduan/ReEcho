#include "UI/ReEchoMinimapCanvasWidget.h"

#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "Rendering/DrawElements.h"
#include "SlateCore.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/SCompoundWidget.h"

namespace
{
/** 小地图固定像素尺寸。 */
constexpr float MinimapCanvasSize = 220.0f;
constexpr float EchoIconSize = 30.0f;
constexpr float PlayerIconSize = 34.0f;
constexpr float FallbackTrailThickness = 2.0f;
constexpr TCHAR InkTrailMaterialPath[] = TEXT("/Game/ReEcho/Materials/UI/M_UI_MinimapInkTrail.M_UI_MinimapInkTrail");
constexpr TCHAR InkBrushTipTexturePath[] =
    TEXT("/Game/ReEcho/Textures/UI/CombatHud/MinimapInkBrush/T_UI_MinimapInkBrushTip.T_UI_MinimapInkBrushTip");
constexpr TCHAR InkGrainTexturePath[] =
    TEXT("/Game/ReEcho/Textures/UI/CombatHud/MinimapInkBrush/T_UI_MinimapInkGrain.T_UI_MinimapInkGrain");
}

void SReEchoMinimapCanvas::Construct(const FArguments& InArgs)
{
	View = FReEchoMinimapView();
	// 只绘制、不拦截鼠标，避免影响 game input。
	SetVisibility(EVisibility::HitTestInvisible);
}

void SReEchoMinimapCanvas::SetInkTrailSettings(const TArray<UMaterialInterface*>& InMaterials,
                                               const float InStampSizePx,
                                               const float InStampSpacingPx,
                                               const float InAngleJitterDegrees,
                                               const float InOpacityJitter,
                                               const TArray<FLinearColor>& InColors,
                                               const int32 InMaxStampsPerEcho)
{
	InkTrailMaterials.Reset(InMaterials.Num());
	for (UMaterialInterface* Material : InMaterials)
	{
		InkTrailMaterials.Add(Material);
	}
	InkTrailStampSizePx = FMath::Clamp(InStampSizePx, 1.0f, 24.0f);
	InkTrailStampSpacingPx = FMath::Clamp(InStampSpacingPx, 0.25f, 12.0f);
	InkTrailAngleJitterDegrees = FMath::Clamp(InAngleJitterDegrees, 0.0f, 180.0f);
	InkTrailOpacityJitter = FMath::Clamp(InOpacityJitter, 0.0f, 1.0f);
	InkTrailColors = InColors;
	MaxInkTrailStampsPerEcho = FMath::Clamp(InMaxStampsPerEcho, 16, 4096);
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

TArray<FReEchoMinimapInkStamp> SReEchoMinimapCanvas::BuildInkTrailStamps(const TArray<FVector2D>& LocalPoints,
                                                                         const float StampSpacingPx,
                                                                         const float AngleJitterDegrees,
                                                                         const float OpacityJitter,
                                                                         const int32 RandomSeed,
                                                                         const int32 MaxStamps)
{
	TArray<FReEchoMinimapInkStamp> Stamps;
	if (LocalPoints.Num() < 2 || MaxStamps <= 0)
	{
		return Stamps;
	}

	const float SafeSpacing = FMath::Max(StampSpacingPx, 0.25f);
	const float SafeAngleJitter = FMath::Clamp(AngleJitterDegrees, 0.0f, 180.0f);
	const float SafeOpacityJitter = FMath::Clamp(OpacityJitter, 0.0f, 1.0f);
	FRandomStream Random(RandomSeed);
	float DistanceToNextStamp = 0.0f;
	Stamps.Reserve(FMath::Min(MaxStamps, 256));

	for (int32 PointIndex = 1; PointIndex < LocalPoints.Num() && Stamps.Num() < MaxStamps; ++PointIndex)
	{
		const FVector2D SegmentStart = LocalPoints[PointIndex - 1];
		const FVector2D Segment = LocalPoints[PointIndex] - SegmentStart;
		const double SegmentLength = Segment.Size();
		if (SegmentLength <= UE_SMALL_NUMBER)
		{
			continue;
		}

		const FVector2D Direction = Segment / SegmentLength;
		const float BaseAngleRadians = FMath::Atan2(Direction.Y, Direction.X);
		while (DistanceToNextStamp <= SegmentLength && Stamps.Num() < MaxStamps)
		{
			FReEchoMinimapInkStamp& Stamp = Stamps.AddDefaulted_GetRef();
			Stamp.Position = SegmentStart + Direction * DistanceToNextStamp;
			Stamp.AngleRadians =
			    BaseAngleRadians + FMath::DegreesToRadians(Random.FRandRange(-SafeAngleJitter, SafeAngleJitter));
			Stamp.Opacity = 1.0f - Random.FRandRange(0.0f, SafeOpacityJitter);
			DistanceToNextStamp += SafeSpacing;
		}
		DistanceToNextStamp -= static_cast<float>(SegmentLength);
	}

	return Stamps;
}

FLinearColor SReEchoMinimapCanvas::ResolveInkTrailColor(const TArray<FLinearColor>& Colors,
                                                        const int32 EchoIndex,
                                                        const FLinearColor& FallbackColor)
{
	return Colors.IsValidIndex(EchoIndex) ? Colors[EchoIndex] : FallbackColor;
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

	// 白色实心刷只用于图标缺失时的安全降级点；画布本身保持透明，
	// 让 WBP 中的正式回响边框素材决定底板视觉。
	FSlateBrush SolidBrush;
	SolidBrush.TintColor = FSlateColor(FLinearColor::White);
	auto DrawMarker = [&](const FVector2D& Center,
	                      const float MarkerSize,
	                      UTexture2D* Icon,
	                      const FLinearColor& FallbackColor,
	                      const int32 MarkerLayer)
	{
		const FVector2f DrawSize(MarkerSize, MarkerSize);
		const FVector2f DrawPosition(static_cast<float>(Center.X - MarkerSize * 0.5f),
		                             static_cast<float>(Center.Y - MarkerSize * 0.5f));
		if (IsValid(Icon))
		{
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(Icon);
			IconBrush.ImageSize = DrawSize;
			FSlateDrawElement::MakeBox(
			    OutDrawElements,
			    MarkerLayer,
			    AllottedGeometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(1.0f, DrawPosition)),
			    &IconBrush,
			    ESlateDrawEffect::None,
			    FLinearColor::White);
			return;
		}
		FSlateDrawElement::MakeBox(
		    OutDrawElements,
		    MarkerLayer,
		    AllottedGeometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(1.0f, DrawPosition)),
		    &SolidBrush,
		    ESlateDrawEffect::None,
		    FallbackColor);
	};

	if (View.bValid && View.ArenaHalfExtents.X > KINDA_SMALL_NUMBER && View.ArenaHalfExtents.Y > KINDA_SMALL_NUMBER)
	{
		// 各回响轨迹折线 + 当前点
		for (int32 EchoIndex = 0; EchoIndex < View.Echoes.Num(); ++EchoIndex)
		{
			const FReEchoMinimapEchoEntry& Entry = View.Echoes[EchoIndex];
			const FLinearColor TrailColor = ResolveInkTrailColor(InkTrailColors, EchoIndex, Entry.Color);
			TArray<FVector2D> Points;
			Points.Reserve(Entry.PathPoints.Num());
			for (const FVector2D& WorldXY : Entry.PathPoints)
			{
				Points.Add(ToLocal(WorldXY, Size));
			}
			if (Points.Num() >= 2)
			{
				UMaterialInterface* Material =
				    InkTrailMaterials.IsValidIndex(EchoIndex) ? InkTrailMaterials[EchoIndex].Get() : nullptr;
				if (Material)
				{
					FSlateBrush InkBrush;
					InkBrush.SetResourceObject(Material);
					InkBrush.ImageSize = FVector2f(InkTrailStampSizePx, InkTrailStampSizePx);
					const TArray<FReEchoMinimapInkStamp> Stamps = BuildInkTrailStamps(Points,
					                                                                  InkTrailStampSpacingPx,
					                                                                  InkTrailAngleJitterDegrees,
					                                                                  InkTrailOpacityJitter,
					                                                                  0x4D494E49 + EchoIndex * 7919,
					                                                                  MaxInkTrailStampsPerEcho);
					for (const FReEchoMinimapInkStamp& Stamp : Stamps)
					{
						const FVector2f DrawSize(InkTrailStampSizePx, InkTrailStampSizePx);
						const FVector2f DrawPosition(static_cast<float>(Stamp.Position.X - InkTrailStampSizePx * 0.5f),
						                             static_cast<float>(Stamp.Position.Y - InkTrailStampSizePx * 0.5f));
						FLinearColor StampColor = FLinearColor::White;
						StampColor.A = TrailColor.A * Stamp.Opacity;
						FSlateDrawElement::MakeRotatedBox(
						    OutDrawElements,
						    LayerId + 2,
						    AllottedGeometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(1.0f, DrawPosition)),
						    &InkBrush,
						    ESlateDrawEffect::None,
						    Stamp.AngleRadians,
						    TOptional<FVector2f>(),
						    FSlateDrawElement::RelativeToElement,
						    StampColor);
					}
				}
				else
				{
					FSlateDrawElement::MakeLines(OutDrawElements,
					                             LayerId + 2,
					                             AllottedGeometry.ToPaintGeometry(),
					                             Points,
					                             ESlateDrawEffect::None,
					                             TrailColor,
					                             true,
					                             FallbackTrailThickness);
				}
			}
			const FVector2D EchoPos = ToLocal(Entry.CurrentLocation, Size);
			DrawMarker(EchoPos, EchoIconSize, Entry.Icon, Entry.Color, LayerId + 3);
		}

		// 玩家当前图标；图标缺失时保留原高亮绿点作为安全降级。
		const FVector2D PlayerPos = ToLocal(View.PlayerLocation, Size);
		DrawMarker(PlayerPos, PlayerIconSize, View.PlayerIcon, FLinearColor(0.2f, 1.0f, 0.4f, 1.0f), LayerId + 4);
	}

	return LayerId + 5;
}

UReEchoMinimapCanvasWidget::UReEchoMinimapCanvasWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	InkTrailColors = {FLinearColor::Red,
	                  FLinearColor::Green,
	                  FLinearColor::Blue,
	                  FLinearColor::Yellow,
	                  FLinearColor(0.0f, 1.0f, 1.0f),
	                  FLinearColor(1.0f, 0.0f, 1.0f)};
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(InkTrailMaterialPath);
	InkTrailMaterial = MaterialAsset.Object;
}

TSharedRef<SWidget> UReEchoMinimapCanvasWidget::RebuildWidget()
{
	RefreshInkTrailMaterials();
	Canvas = SNew(SReEchoMinimapCanvas);
	ApplyInkTrailSettings();
	return Canvas.ToSharedRef();
}

void UReEchoMinimapCanvasWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RefreshInkTrailMaterials();
	ApplyInkTrailSettings();
}

void UReEchoMinimapCanvasWidget::RefreshInkTrailMaterials()
{
	if (!InkTrailMaterial)
	{
		RuntimeInkTrailMaterials.Reset();
		return;
	}

	RuntimeInkTrailMaterials.SetNum(InkTrailColors.Num());
	for (int32 ColorIndex = 0; ColorIndex < InkTrailColors.Num(); ++ColorIndex)
	{
		TObjectPtr<UMaterialInstanceDynamic>& Material = RuntimeInkTrailMaterials[ColorIndex];
		if (!Material)
		{
			Material = UMaterialInstanceDynamic::Create(InkTrailMaterial, this);
		}
		if (Material)
		{
			Material->SetScalarParameterValue(TEXT("GrainStrength"), FMath::Clamp(InkTrailGrainStrength, 0.0f, 1.0f));
			Material->SetVectorParameterValue(TEXT("TrailColor"), InkTrailColors[ColorIndex]);
		}
	}
}

void UReEchoMinimapCanvasWidget::ApplyInkTrailSettings()
{
	if (!Canvas.IsValid())
	{
		return;
	}
	TArray<UMaterialInterface*> Materials;
	Materials.Reserve(RuntimeInkTrailMaterials.Num());
	for (const TObjectPtr<UMaterialInstanceDynamic>& Material : RuntimeInkTrailMaterials)
	{
		Materials.Add(Material.Get());
	}
	Canvas->SetInkTrailSettings(Materials,
	                            InkTrailStampSizePx,
	                            InkTrailStampSpacingPx,
	                            InkTrailAngleJitterDegrees,
	                            InkTrailOpacityJitter,
	                            InkTrailColors,
	                            MaxInkTrailStampsPerEcho);
}

void UReEchoMinimapCanvasWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Canvas.Reset();
	RuntimeInkTrailMaterials.Reset();
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

const TCHAR* UReEchoMinimapCanvasWidget::GetInkTrailMaterialPath()
{
	return InkTrailMaterialPath;
}

const TCHAR* UReEchoMinimapCanvasWidget::GetInkBrushTipTexturePath()
{
	return InkBrushTipTexturePath;
}

const TCHAR* UReEchoMinimapCanvasWidget::GetInkGrainTexturePath()
{
	return InkGrainTexturePath;
}
