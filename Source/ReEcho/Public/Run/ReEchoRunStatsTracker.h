#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ReEchoRunStatsTracker.generated.h"

struct FReEchoDamageEvent;

/**
 * 整局战斗统计快照。
 *
 * 这是一套独立于卡牌运行时（FReEchoCardRuntimeState）与战斗结算的展示用统计：
 * 它不参与任何玩法判定，不受每关重置或卡牌消耗清零影响，只随一局游戏累计，供结算界面读取。
 */
USTRUCT(BlueprintType)
struct REECHO_API FReEchoRunCombatStats
{
	GENERATED_BODY()

	/** 回响累计造成的实际伤害；元素反应伤害按触发者归入本体或回响。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float EchoDamageTotal = 0.0f;

	/** 本体累计造成的实际伤害；元素反应伤害按触发者归入本体或回响。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float PlayerDamageTotal = 0.0f;

	/** 元素反应触发次数，整局累计。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ReactionTotal = 0;

	/** 最高单次实际伤害，来源不限（含元素反应伤害）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float MaxSingleHitDamage = 0.0f;

	/** 击杀怪物总数，整局累计；敌人自身来源导致的死亡不计入。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 KillTotal = 0;

	/** 恢复初始值；开新一局时调用。 */
	void Reset()
	{
		EchoDamageTotal = 0.0f;
		PlayerDamageTotal = 0.0f;
		ReactionTotal = 0;
		MaxSingleHitDamage = 0.0f;
		KillTotal = 0;
	}
};

/**
 * 整局战斗统计的唯一所有者。
 *
 * 与卡牌、商店、存档体系完全解耦：它只被动接收转发来的战斗事件并累加，
 * 不写入任何玩法状态，也不被任何玩法逻辑读取，因此不会影响既有行为。
 */
UCLASS()
class REECHO_API UReEchoRunStatsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 开新一局时清零全部累计值。 */
	void ResetRunStats();

	/** 记录一次敌人受伤；按伤害来源与攻击发起者归类到本体或回响。 */
	void RecordEnemyHurt(const FReEchoDamageEvent& Event);

	/** 记录一次敌人死亡；敌人自身来源导致的死亡不计入击杀。 */
	void RecordEnemyDeath(const FReEchoDamageEvent& Event);

	/** 记录一次元素反应触发。 */
	void RecordElementReaction();

	const FReEchoRunCombatStats& GetRunStats() const
	{
		return Stats;
	}

private:
	UPROPERTY()
	FReEchoRunCombatStats Stats;
};
