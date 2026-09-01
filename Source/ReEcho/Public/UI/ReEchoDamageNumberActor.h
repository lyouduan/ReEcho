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

	/**
	 * 创建并初始化一次伤害数字表现。
	 * @param bCritical 是否使用蓝图配置的暴击字号缩放。
	 */
	static void SpawnDamageNumber(UWorld* World,
	                              const FVector& WorldLocation,
	                              float Damage,
	                              const FLinearColor& Color,
	                              bool bCritical = false);

	/** 伤害数字专用运行时字体资产路径，供构造与自动化验证共享。 */
	static const TCHAR* GetDamageNumberFontPath();
	/** 支持顶点 Alpha 的半透明 TextRender 材质。 */
	static const TCHAR* GetDamageNumberMaterialPath();
	/** 美术可编辑的伤害跳字 Blueprint Class 路径。 */
	static const TCHAR* GetDamageNumberBlueprintClassPath();
	/** 收集首次进入战斗前需要常驻的跳字类、字体和材质。 */
	static void GatherPreloadAssetPaths(TArray<FString>& OutPaths);
#if WITH_DEV_AUTOMATION_TESTS
	static int32 GetPooledCountForTests(UWorld* World);
#endif

private:
	void InitializeDamage(float Damage, const FLinearColor& Color, bool bCritical = false);
	void ReturnToPool();

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

	/** 暴击数字相对普通数字的视觉字号倍率，不改变实际伤害。 */
	UPROPERTY(EditDefaultsOnly,
	          BlueprintReadOnly,
	          Category = "Damage Number|Critical",
	          meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float CriticalSizeScale = 1.35f;

	FLinearColor InitialColor = FLinearColor::White;
};
