#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ReEchoBalanceSettings.generated.h"

UENUM(BlueprintType)
enum class EReEchoWeaponSlot : uint8
{
	None = 0,
	PhysicalOrb = 1,
	Sword = 2,
	ElementalOrb = 3
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoWeaponConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EReEchoWeaponSlot Slot = EReEchoWeaponSlot::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float Interval = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float Range = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float PhysicalCoefficient = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float ElementalCoefficient = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float ArcDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor ProjectileColor = FLinearColor::White;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "ReEcho Balance"))
class REECHO_API UReEchoBalanceSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/** 一轮游戏包含的遭遇总数；最后一场生成 Boss 并决定最终结算。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 TotalEncounterCount = 6;

	int32 GetTotalEncounterCount() const { return FMath::Max(1, TotalEncounterCount); }

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

	/** 第一场遭遇的普通怪数量，后续关卡会继续增加。 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 BaseGruntCount = 10;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0"))
	int32 GruntsPerEncounter = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 MaxGruntCount = 18;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0"))
	int32 MaxBomberCount = 6;

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
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TArray<FReEchoWeaponConfig> Weapons;
};
