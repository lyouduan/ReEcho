#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReEchoDamageNumberActor.generated.h"

class UTextRenderComponent;

/** 在受击位置短暂显示并向上飘动的世界空间伤害数字。 */
UCLASS()

class REECHO_API AReEchoDamageNumberActor : public AActor
{
	GENERATED_BODY()

public:
	AReEchoDamageNumberActor();

	virtual void Tick(float DeltaSeconds) override;

	/** 创建并初始化一次伤害数字表现。 */
	static void SpawnDamageNumber(UWorld* World, const FVector& WorldLocation, float Damage, const FLinearColor& Color);

	/** 伤害数字专用运行时字体资产路径，供构造与自动化验证共享。 */
	static const TCHAR* GetDamageNumberFontPath();
	/** 支持顶点 Alpha 的半透明 TextRender 材质。 */
	static const TCHAR* GetDamageNumberMaterialPath();
	/** 美术可编辑的伤害跳字 Blueprint Class 路径。 */
	static const TCHAR* GetDamageNumberBlueprintClassPath();
	/** 上漂生命周期内的线性透明度，超出生命周期后为零。 */
	static float CalculateOpacity(float ElapsedSeconds, float DurationSeconds);
	/** 支持延迟与指数曲线的透明度配置。 */
	static float CalculateOpacityProfile(
	    float ElapsedSeconds, float DurationSeconds, float FadeStartTimeSeconds, float FadeCurveExponent);

private:
	void InitializeDamage(float Damage, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Text;

	float ElapsedTime = 0.0f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Damage Number|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.05", Units = "s"))
	float DisplayDuration = 0.9f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Damage Number|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float FadeStartTime = 0.0f;

	/** 1 为线性；大于 1 时保持更久后淡出，小于 1 时更早淡出。 */
	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Damage Number|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.05"))
	float FadeCurveExponent = 1.0f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Damage Number|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm/s"))
	float FloatSpeed = 70.0f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Damage Number|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float StartScale = 1.15f;

	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Damage Number|Animation",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float EndScale = 0.85f;

	FLinearColor InitialColor = FLinearColor::White;
};
