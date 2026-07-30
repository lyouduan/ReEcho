#include "UI/ReEchoWeatherWidget.h"

#include "Rendering/DrawElements.h"

namespace
{
constexpr int32 RainStreakCount = 96;
constexpr int32 FogLayerCount = 4;
}

void UReEchoWeatherWidget::SetWeatherScene(const EReEchoWeatherScene NewWeatherScene)
{
	WeatherScene = NewWeatherScene;
	AnimationTime = 0.0f;
	InvalidateLayoutAndVolatility();
}

void UReEchoWeatherWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AnimationTime += InDeltaTime;
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

	TArray<FSlateGradientStop> BaseFogStops;
	BaseFogStops.Emplace(FVector2f(0.0f, 0.0f), FLinearColor(0.72f, 0.79f, 0.82f, 0.025f));
	BaseFogStops.Emplace(FVector2f(Size.X * 0.5f, Size.Y * 0.55f), FLinearColor(0.75f, 0.81f, 0.83f, 0.055f));
	BaseFogStops.Emplace(FVector2f(Size.X, Size.Y), FLinearColor(0.78f, 0.82f, 0.83f, 0.11f));
	FSlateDrawElement::MakeGradient(OutDrawElements,
	                                BaseLayer + 1,
	                                AllottedGeometry.ToPaintGeometry(),
	                                MoveTemp(BaseFogStops),
	                                EOrientation::Orient_Vertical);

	for (int32 LayerIndex = 0; LayerIndex < FogLayerCount; ++LayerIndex)
	{
		const float NormalizedLayer = static_cast<float>(LayerIndex) / static_cast<float>(FogLayerCount - 1);
		const float Drift = FMath::Sin(AnimationTime * (0.07f + NormalizedLayer * 0.025f) + LayerIndex * 1.91f);
		const float CenterY = Size.Y * FMath::Lerp(0.25f, 0.88f, NormalizedLayer) + Drift * Size.Y * 0.018f;
		const float HalfWidth = Size.Y * FMath::Lerp(0.18f, 0.25f, NormalizedLayer);
		const float LowerY = FMath::Clamp(CenterY - HalfWidth, 0.0f, Size.Y);
		const float UpperY = FMath::Clamp(CenterY + HalfWidth, 0.0f, Size.Y);
		const float PeakAlpha = FMath::Lerp(0.025f, 0.055f, NormalizedLayer);

		TArray<FSlateGradientStop> LayerStops;
		LayerStops.Emplace(FVector2f(0.0f, 0.0f), FLinearColor(0.76f, 0.81f, 0.82f, 0.0f));
		LayerStops.Emplace(FVector2f(0.0f, LowerY), FLinearColor(0.76f, 0.81f, 0.82f, 0.0f));
		LayerStops.Emplace(FVector2f(Size.X * 0.5f, CenterY), FLinearColor(0.78f, 0.83f, 0.84f, PeakAlpha));
		LayerStops.Emplace(FVector2f(Size.X, UpperY), FLinearColor(0.76f, 0.81f, 0.82f, 0.0f));
		LayerStops.Emplace(FVector2f(Size.X, Size.Y), FLinearColor(0.76f, 0.81f, 0.82f, 0.0f));
		FSlateDrawElement::MakeGradient(OutDrawElements,
		                                BaseLayer + 2 + LayerIndex,
		                                AllottedGeometry.ToPaintGeometry(),
		                                MoveTemp(LayerStops),
		                                EOrientation::Orient_Vertical);
	}
	return BaseLayer + 1;
}
