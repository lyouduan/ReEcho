#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/SCompoundWidget.h"
#include "ReEchoMinimapCanvasWidget.generated.h"

class UTexture2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/** One deterministic brush-tip stamp sampled along a projected minimap trail. */
struct FReEchoMinimapInkStamp
{
	FVector2D Position = FVector2D::ZeroVector;
	float AngleRadians = 0.0f;
	float Opacity = 1.0f;
};

/**
 * 单条回响轨迹在小地图上的绘制数据。
 * 轨迹点已是世界 XY 平面坐标，由 GameMode 从录制数据转换后填入，
 * 小地图控件本身与录制结构解耦，只负责投影与绘制。
 */
struct FReEchoMinimapEchoEntry
{
	FVector2D CurrentLocation = FVector2D::ZeroVector;
	TArray<FVector2D> PathPoints;
	FLinearColor Color = FLinearColor::White;
	UTexture2D* Icon = nullptr;
};

/** 小地图一帧的完整视图，由 GameMode::BuildMinimapView 填充。 */
struct FReEchoMinimapView
{
	FVector2D ArenaCenter = FVector2D::ZeroVector;
	FVector2D ArenaHalfExtents = FVector2D::ZeroVector;
	FVector2D PlayerLocation = FVector2D::ZeroVector;
	UTexture2D* PlayerIcon = nullptr;
	TArray<FReEchoMinimapEchoEntry> Echoes;
	bool bValid = false;
};

/** 透明 Slate 画布：在 OnPaint 中绘制回响轨迹折线及 Player/Echo 实时图标。 */
class SReEchoMinimapCanvas : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SReEchoMinimapCanvas)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetInkTrailSettings(UMaterialInterface* InMaterial,
	                         float InStampSizePx,
	                         float InStampSpacingPx,
	                         float InAngleJitterDegrees,
	                         float InOpacityJitter,
	                         int32 InMaxStampsPerEcho);

	/** 设置本帧要绘制的数据视图。 */
	void SetView(const FReEchoMinimapView& InView)
	{
		View = InView;
	}

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

	virtual int32 OnPaint(const FPaintArgs& Args,
	                      const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect,
	                      FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId,
	                      const FWidgetStyle& InWidgetStyle,
	                      bool bParentEnabled) const override;

	/** 世界 XY -> 小地图局部像素坐标。纯函数，供单元测试。 */
	static FVector2D TransformWorldToMinimap(const FVector2D& WorldXY,
	                                         const FVector2D& Center,
	                                         const FVector2D& HalfExtents,
	                                         const FVector2D& CanvasSize);
	/** Resample a projected polyline into stable, bounded brush-tip stamps. */
	static TArray<FReEchoMinimapInkStamp> BuildInkTrailStamps(const TArray<FVector2D>& LocalPoints,
	                                                          float StampSpacingPx,
	                                                          float AngleJitterDegrees,
	                                                          float OpacityJitter,
	                                                          int32 RandomSeed,
	                                                          int32 MaxStamps);

private:
	FReEchoMinimapView View;
	TWeakObjectPtr<UMaterialInterface> InkTrailMaterial;
	float InkTrailStampSizePx = 8.0f;
	float InkTrailStampSpacingPx = 1.5f;
	float InkTrailAngleJitterDegrees = 18.0f;
	float InkTrailOpacityJitter = 0.16f;
	int32 MaxInkTrailStampsPerEcho = 1024;

	FVector2D ToLocal(const FVector2D& WorldXY, const FVector2D& CanvasSize) const;
};

/**
 * 可在 UMG 设计器（WBP）中放置的小地图画布控件。
 * 内部持有一个 SReEchoMinimapCanvas 裸 Slate 控件负责矢量绘制；
 * 本控件的 Slot（锚点 / 位置 / 大小）完全由所在 WBP 决定，
 * 从而支持在编辑器里拖拽布局，而不是在 C++ 里硬编码。
 *
 * 典型用法：在 WBP_ReEchoEncounterHud 中放入本控件，
 * 由 UReEchoEncounterHudWidget::SetMinimapView 每帧转发 GameMode 的 FReEchoMinimapView。
 */
UCLASS()

class REECHO_API UReEchoMinimapCanvasWidget : public UWidget
{
	GENERATED_BODY()

public:
	UReEchoMinimapCanvasWidget(const FObjectInitializer& ObjectInitializer);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/** 由外层 HUD Widget 每帧调用，转发给内部 Slate 画布。 */
	void SetView(const FReEchoMinimapView& InView);

	/** 暴露坐标映射给单元测试。 */
	static FVector2D TransformWorldToMinimap(const FVector2D& WorldXY,
	                                         const FVector2D& Center,
	                                         const FVector2D& HalfExtents,
	                                         const FVector2D& CanvasSize);
	static const TCHAR* GetInkTrailMaterialPath();
	static const TCHAR* GetInkBrushTipTexturePath();
	static const TCHAR* GetInkGrainTexturePath();

protected:
	virtual void SynchronizeProperties() override;

	TSharedPtr<SReEchoMinimapCanvas> Canvas;

private:
	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Minimap|Ink Trail",
	          meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "24.0"))
	float InkTrailStampSizePx = 8.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Minimap|Ink Trail",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.25", ClampMax = "12.0"))
	float InkTrailStampSpacingPx = 1.5f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Minimap|Ink Trail",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float InkTrailAngleJitterDegrees = 18.0f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Minimap|Ink Trail",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float InkTrailOpacityJitter = 0.16f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          Category = "Minimap|Ink Trail",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float InkTrailGrainStrength = 0.65f;

	UPROPERTY(EditAnywhere,
	          BlueprintReadWrite,
	          AdvancedDisplay,
	          Category = "Minimap|Ink Trail",
	          meta = (AllowPrivateAccess = "true", ClampMin = "16", ClampMax = "4096"))
	int32 MaxInkTrailStampsPerEcho = 1024;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Minimap|Ink Trail", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> InkTrailMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RuntimeInkTrailMaterial;
};
