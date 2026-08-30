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
	 * @param SizeScale 字号缩放（基准 WorldSize=52，暴击时 = 1 + CriticalEffect）。
	 * @param bUnderline 是否显示下划线（暴击特殊反馈）。
	 */
	static void SpawnDamageNumber(UWorld* World, const FVector& WorldLocation, float Damage, const FLinearColor& Color, float SizeScale = 1.0f, bool bUnderline = false);

	/** 伤害数字专用运行时字体资产路径，供构造与自动化验证共享。 */
	static const TCHAR* GetDamageNumberFontPath();
	/** 支持顶点 Alpha 的半透明 TextRender 材质。 */
	static const TCHAR* GetDamageNumberMaterialPath();
	/** 美术可编辑的伤害跳字 Blueprint Class 路径。 */
	static const TCHAR* GetDamageNumberBlueprintClassPath();

private:
	void InitializeDamage(float Damage, const FLinearColor& Color, float SizeScale = 1.0f, bool bUnderline = false);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Text;

	/** 暴击下划线：与数字等宽的 '_' 串，定位在数字正下方。 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Underline;

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

	FLinearColor InitialColor = FLinearColor::White;
};
