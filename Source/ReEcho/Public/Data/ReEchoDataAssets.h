#pragma once

#include "CoreMinimal.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/DataAsset.h"
#include "ReEchoDataAssets.generated.h"

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCharacterAliasRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色")
	FName AliasId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "角色")
	FName CanonicalCharacterId = NAME_None;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoCoreDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "运行时校验")
	TArray<FReEchoRuntimeSmokeRow> RuntimeSmokeRows;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "角色")
	TArray<FReEchoCsvCharacterRow> Characters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "角色")
	TArray<FReEchoCharacterAliasRow> CharacterAliases;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "角色")
	TArray<FReEchoCsvCharacterAbilityRow> CharacterAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "属性")
	TArray<FReEchoCsvAttributeRow> Attributes;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoCardDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "卡牌")
	FString DomainRevision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "卡牌")
	TArray<FReEchoCsvCardRow> Cards;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoElementDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "元素")
	TArray<FReEchoCsvElementRow> Elements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "状态")
	TArray<FReEchoCsvStatusRow> Statuses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "反应")
	TArray<FReEchoCsvReactionRow> Reactions;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoWeaponDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "武器")
	FString DomainRevision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "武器")
	TArray<FReEchoCsvWeaponTypeRow> WeaponTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "武器")
	TArray<FReEchoCsvWeaponRow> Weapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "武器")
	TArray<FReEchoCsvAttackStepRow> AttackSteps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "配件槽")
	TArray<FReEchoCsvSlotTypeRow> SlotTypes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "配件槽")
	TArray<FReEchoCsvSlotProfileRow> SlotProfiles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "配件")
	TArray<FReEchoCsvPartRow> Parts;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoEnemyDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "怪物")
	TArray<FReEchoCsvEnemyRow> Enemies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "怪物")
	TArray<FReEchoCsvEnemyCombatStatRow> EnemyCombatStats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "怪物掉落")
	TArray<FReEchoCsvEnemyShardDropRow> EnemyShardDrops;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoEncounterDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "阶段")
	TArray<FReEchoCsvStageRow> Stages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "遭遇")
	TArray<FReEchoCsvEncounterRow> Encounters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "遭遇")
	TArray<FReEchoCsvEncounterWaveRow> EncounterWaves;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "出生")
	TArray<FReEchoCsvSpawnProfileRow> SpawnProfiles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "出生")
	TArray<FReEchoCsvSpawnPolicyRow> SpawnPolicies;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoShopDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "商店")
	TArray<FReEchoCsvShopPriceRangeRow> PriceRanges;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "商店")
	TArray<FReEchoCsvShopDropLevelRow> DropLevels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "商店")
	TArray<FReEchoCsvShopRefreshRuleRow> RefreshRules;
};

UCLASS(BlueprintType)
class REECHO_API UReEchoGameDataCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "数据域")
	TObjectPtr<UReEchoCoreDataAsset> Core;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "数据域")
	TObjectPtr<UReEchoCardDataAsset> Cards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "数据域")
	TObjectPtr<UReEchoElementDataAsset> Elements;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "数据域")
	TObjectPtr<UReEchoWeaponDataAsset> Weapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "数据域")
	TObjectPtr<UReEchoEnemyDataAsset> Enemies;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "数据域")
	TObjectPtr<UReEchoEncounterDataAsset> Encounters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "数据域")
	TObjectPtr<UReEchoShopDataAsset> Shop;
};
