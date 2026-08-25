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
	/** 上漂生命周期内的线性透明度，超出生命周期后为零。 */
	static float CalculateOpacity(float ElapsedSeconds, float DurationSeconds);

private:
	void InitializeDamage(float Damage, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Text;

	float ElapsedTime = 0.0f;
	float DisplayDuration = 0.9f;
	FLinearColor InitialColor = FLinearColor::White;
};
