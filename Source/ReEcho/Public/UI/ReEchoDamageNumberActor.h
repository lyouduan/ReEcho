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
	static void SpawnDamageNumber(
		UWorld* World,
		const FVector& WorldLocation,
		float Damage,
		const FLinearColor& Color);

private:
	void InitializeDamage(float Damage, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Text;

	float ElapsedTime = 0.0f;
	float DisplayDuration = 0.9f;
	FLinearColor InitialColor = FLinearColor::White;
};
