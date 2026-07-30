#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoWeatherWidget.generated.h"

UENUM()
enum class EReEchoWeatherScene : uint8
{
	Clear,
	Rain,
	Fog
};

/** Full-screen, input-transparent weather presentation for the fixed arena camera. */
UCLASS()

class REECHO_API UReEchoWeatherWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetWeatherScene(EReEchoWeatherScene NewWeatherScene);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args,
	                          const FGeometry& AllottedGeometry,
	                          const FSlateRect& MyCullingRect,
	                          FSlateWindowElementList& OutDrawElements,
	                          int32 LayerId,
	                          const FWidgetStyle& InWidgetStyle,
	                          bool bParentEnabled) const override;

private:
	EReEchoWeatherScene WeatherScene = EReEchoWeatherScene::Clear;
	float AnimationTime = 0.0f;
};
