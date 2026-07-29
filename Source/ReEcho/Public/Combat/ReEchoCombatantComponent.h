#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoCombatantComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReEchoHealthChanged, float, Current, float, Maximum);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoDeath);

/** 通用战斗属性组件：管理生命值、受伤结算和死亡事件。 */
UCLASS(ClassGroup = (ReEcho), meta = (BlueprintSpawnableComponent))
class REECHO_API UReEchoCombatantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UReEchoCombatantComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FReEchoStatBlock Stats;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float CurrentHealth = 100.f;

	UPROPERTY(BlueprintAssignable)
	FReEchoHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FReEchoDeath OnDeath;

	UFUNCTION(BlueprintCallable)
	void InitializeFromStats(const FReEchoStatBlock& InStats, bool bFillHealth = true);

	/** 扣除已经过战斗规则计算的最终伤害，并返回实际扣除值。 */
	UFUNCTION(BlueprintCallable)
	float ApplyFinalDamage(float Damage);

	UFUNCTION(BlueprintPure)
	bool IsAlive() const;

protected:
	virtual void BeginPlay() override;
};
