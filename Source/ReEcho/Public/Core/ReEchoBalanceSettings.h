#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ReEchoBalanceSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ReEcho Balance"))

class REECHO_API UReEchoBalanceSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/** 仅在 Encounter Catalog 不可用时使用的兼容回退；正常运行以 encounters.csv 为唯一权威。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 TotalEncounterCount = 8;

	int32 GetTotalEncounterCount() const
	{
		return FMath::Max(1, TotalEncounterCount);
	}

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	float EncounterDuration = 30.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	float SetupDuration = 3.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Simulation")
	float FixedStepHz = 60.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Recording")
	float RecordingHz = 20.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HpBase = 25.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float HpPerPoint = 5.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float BaseMoveSpeed = 210.f;
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Echo")
	int32 RecordingHistoryLimit = 3;

	/** 场景背景的世界高度；相机宽度会按背景纹理宽高比自动同步。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Arena", meta = (ClampMin = "100.0"))
	float ArenaSceneWorldHeight = 6300.0f;

	/** 使用屏幕空间下雨表现的遭遇编号（从 1 开始）。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Weather")
	TArray<int32> RainEncounterIndices;

	/** 使用屏幕空间雾气表现的遭遇编号（从 1 开始）；与雨重叠时雨优先。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Weather")
	TArray<int32> FogEncounterIndices;

	/** 第一场遭遇的普通怪数量，后续关卡会继续增加。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 BaseGruntCount = 10;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0"))
	int32 GruntsPerEncounter = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 MaxGruntCount = 18;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0"))
	int32 MaxBomberCount = 6;

	/** 爆破怪进入该范围后点燃引信；应大于实际伤害半径。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter|Bomber", meta = (ClampMin = "0.0"))
	float BomberTriggerRadius = 260.0f;

	/** 爆炸实际伤害半径，不在范围内的玩家不会受伤。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter|Bomber", meta = (ClampMin = "0.0"))
	float BomberDamageRadius = 180.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter|Bomber", meta = (ClampMin = "0.1"))
	float BomberFuseDuration = 1.2f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter|Bomber", meta = (ClampMin = "0.0"))
	float BomberDamage = 22.0f;

	/** 怪物与场景碰撞边界之间保留的安全距离。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0.0"))
	float EnemySpawnEdgeInset;

	/** 怪物相对玩家随机出生的最小距离。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0.0"))
	float EnemySpawnMinPlayerDistance;

	/** 怪物相对玩家随机出生的最大距离。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0.0"))
	float EnemySpawnMaxPlayerDistance;

	/** 默认玩家外观：J_CAT、J_HEART、J_SPADE、J_CLOVER 或 J_DIAMOND。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Characters")
	FName DefaultCharacterId = TEXT("J_CAT");

	/** Project Settings 中可编辑、打包时随 Game 配置发布的运行时武器定义。 */
};
