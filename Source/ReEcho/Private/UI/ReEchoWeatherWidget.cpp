#include "UI/ReEchoWeatherWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"

namespace
{
constexpr int32 RainStreakCount = 96;
constexpr int32 FogMeshColumns = 64;
constexpr int32 FogMeshRows = 36;
constexpr float FogClearRadius = 145.0f;
constexpr float FogFeatherRadius = 190.0f;
constexpr float FogMinimumCoverage = 0.015f;
constexpr float FogMaximumCoverage = 0.955f;

float SmoothStep01(const float Value)
{
	const float Clamped = FMath::Clamp(Value, 0.0f, 1.0f);
	return Clamped * Clamped * (3.0f - 2.0f * Clamped);
}

float ComputeFogCoverage(const FVector2D& Position,
                         const TArray<FVector2D>& RevealCenters,
                         const float AnimationTime)
{
	float CombinedCoverage = 1.0f;
	for (const FVector2D& RevealCenter : RevealCenters)
	{
		const FVector2D Offset = Position - RevealCenter;
		const float Angle = FMath::Atan2(Offset.Y, Offset.X);
		const float BoundaryDrift =
		    10.0f * FMath::Sin(Angle * 3.0f + AnimationTime * 0.22f) +
		    6.0f * FMath::Sin(Angle * 7.0f - AnimationTime * 0.13f);
		const float SourceCoverage =
		    SmoothStep01((Offset.Length() - FogClearRadius - BoundaryDrift) / FogFeatherRadius);

		// Coverage is 1 - transmittance. Multiplication leaves a clear opening around either source.
		CombinedCoverage *= SourceCoverage;
	}
	return CombinedCoverage;
}

float ComputeFogDensityVariation(const FVector2D& Position, const FVector2D& ViewSize, const float AnimationTime)
{
	const FVector2D NormalizedPosition(Position.X / FMath::Max(ViewSize.X, 1.0f),
	                                   Position.Y / FMath::Max(ViewSize.Y, 1.0f));
	const float BroadLayer =
	    FMath::Sin(NormalizedPosition.X * 8.2f + NormalizedPosition.Y * 3.1f + AnimationTime * 0.17f);
	const float CrossingLayer =
	    FMath::Sin(NormalizedPosition.X * -5.7f + NormalizedPosition.Y * 9.4f - AnimationTime * 0.11f);
	return (BroadLayer + CrossingLayer) * 0.018f;
}
}

void UReEchoWeatherWidget::SetWeatherScene(const EReEchoWeatherScene NewWeatherScene)
{
	WeatherScene = NewWeatherScene;
	AnimationTime = 0.0f;
	InvalidateLayoutAndVolatility();
}

void UReEchoWeatherWidget::SetFogRevealSources(AActor* InPlayer, AActor* InEcho)
{
	FogPlayer = InPlayer;
	FogEcho = InEcho;
}

void UReEchoWeatherWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimationTime += InDeltaTime;
	FogRevealCenters.Reset();
	if (WeatherScene == EReEchoWeatherScene::Fog)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			auto AddRevealCenter = [&](const TWeakObjectPtr<AActor>& Source)
			{
				FVector2D ScreenPosition;
				if (Source.IsValid() &&
				    PlayerController->ProjectWorldLocationToScreen(Source->GetActorLocation(), ScreenPosition, true))
				{
					FogRevealCenters.Add(MyGeometry.AbsoluteToLocal(ScreenPosition));
				}
			};
			AddRevealCenter(FogPlayer);
			AddRevealCenter(FogEcho);
		}
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UReEchoWeatherWidget::NativePaint(const FPaintArgs& Args,
                                        const FGeometry& AllottedGeometry,
                                        const FSlateRect& MyCullingRect,
                                        FSlateWindowElementList& OutDrawElements,
                                        const int32 LayerId,
                                        const FWidgetStyle& InWidgetStyle,
                                        const bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(
	    Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	if (WeatherScene == EReEchoWeatherScene::Clear || Size.X <= 1.0f || Size.Y <= 1.0f)
	{
		return BaseLayer;
	}

	if (WeatherScene == EReEchoWeatherScene::Rain)
	{
		for (int32 StreakIndex = 0; StreakIndex < RainStreakCount; ++StreakIndex)
		{
			const float Seed = FMath::Frac(static_cast<float>(StreakIndex) * 0.61803398875f);
			const float Speed = 520.0f + 310.0f * FMath::Frac(Seed * 7.31f);
			const float LocalY = FMath::Fmod(Seed * (Size.Y + 180.0f) + AnimationTime * Speed, Size.Y + 180.0f) - 90.0f;
			const float Drift = FMath::Fmod(AnimationTime * 75.0f, Size.X + 120.0f);
			const float LocalX = FMath::Fmod(Seed * 13.37f * Size.X + Drift, Size.X + 120.0f) - 60.0f;
			const float StreakLength = 18.0f + 24.0f * FMath::Frac(Seed * 19.7f);
			TArray<FVector2f> Points;
			Points.Add(FVector2f(LocalX, LocalY));
			Points.Add(FVector2f(LocalX - StreakLength * 0.35f, LocalY + StreakLength));
			FSlateDrawElement::MakeLines(OutDrawElements,
			                             BaseLayer + 1,
			                             AllottedGeometry.ToPaintGeometry(),
			                             Points,
			                             ESlateDrawEffect::None,
			                             FLinearColor(0.63f, 0.82f, 1.0f, 0.34f),
			                             true,
			                             1.35f);
		}
		return BaseLayer + 1;
	}

	const FSlateBrush* FogBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateResourceHandle FogResourceHandle =
	    FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FogBrush);
	if (!FogResourceHandle.IsValid())
	{
		return BaseLayer;
	}

	const int32 VertexColumns = FogMeshColumns + 1;
	const int32 VertexRows = FogMeshRows + 1;
	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	Vertices.Reserve(VertexColumns * VertexRows);
	Indices.Reserve(FogMeshColumns * FogMeshRows * 6);

	const FSlateRenderTransform& PaintTransform = AllottedGeometry.GetAccumulatedRenderTransform();
	for (int32 Row = 0; Row < VertexRows; ++Row)
	{
		const float VerticalProgress = static_cast<float>(Row) / static_cast<float>(FogMeshRows);
		for (int32 Column = 0; Column < VertexColumns; ++Column)
		{
			const float HorizontalProgress = static_cast<float>(Column) / static_cast<float>(FogMeshColumns);
			const FVector2D Position(HorizontalProgress * Size.X, VerticalProgress * Size.Y);
			const float Coverage = ComputeFogCoverage(Position, FogRevealCenters, AnimationTime);
			const float DensityVariation = ComputeFogDensityVariation(Position, Size, AnimationTime);
			const float FogAlpha =
			    FMath::Clamp(FMath::Lerp(FogMinimumCoverage, FogMaximumCoverage, Coverage) +
			                     DensityVariation * SmoothStep01(Coverage),
			                 FogMinimumCoverage,
			                 0.97f);
			const FColor VertexColor = FLinearColor(0.028f, 0.052f, 0.058f, FogAlpha).ToFColor(true);
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			    PaintTransform,
			    FVector2f(Position),
			    FVector2f(HorizontalProgress, VerticalProgress),
			    VertexColor));
		}
	}

	for (int32 Row = 0; Row < FogMeshRows; ++Row)
	{
		for (int32 Column = 0; Column < FogMeshColumns; ++Column)
		{
			const SlateIndex TopLeft = static_cast<SlateIndex>(Row * VertexColumns + Column);
			const SlateIndex TopRight = static_cast<SlateIndex>(TopLeft + 1);
			const SlateIndex BottomLeft = static_cast<SlateIndex>(TopLeft + VertexColumns);
			const SlateIndex BottomRight = static_cast<SlateIndex>(BottomLeft + 1);
			Indices.Add(TopLeft);
			Indices.Add(TopRight);
			Indices.Add(BottomRight);
			Indices.Add(TopLeft);
			Indices.Add(BottomRight);
			Indices.Add(BottomLeft);
		}
	}

	FSlateDrawElement::MakeCustomVerts(
	    OutDrawElements, BaseLayer + 1, FogResourceHandle, Vertices, Indices, nullptr, 0, 0);
	return BaseLayer + 1;
}
