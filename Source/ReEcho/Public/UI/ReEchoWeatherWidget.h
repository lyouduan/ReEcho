#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoWeatherWidget.generated.h"

class AActor;

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
	/** 切换全屏天气表现；不改变战斗模拟。 */
	void SetWeatherScene(EReEchoWeatherScene NewWeatherScene);

	/** 设置雾中需要保持可见的玩家与回响。 */
	void SetFogRevealSources(AActor* InPlayer, AActor* InEcho);

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
	TWeakObjectPtr<AActor> FogPlayer;
	TWeakObjectPtr<AActor> FogEcho;
	TArray<FVector2D> FogRevealCenters;
};
