#pragma once

#include "CoreMinimal.h"
#include "Cards/ReEchoCardCatalog.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoCsvDataRegistry.generated.h"

class UReEchoGameDataCatalog;

UENUM(BlueprintType)
enum class EReEchoCsvValueOp : uint8
{
	Add,
	Multiply,
	Override
};

UENUM(BlueprintType)
enum class EReEchoElementRole : uint8
{
	Trigger,
	Attachment
};

UENUM(BlueprintType)
enum class EReEchoInputSlot : uint8
{
	None = 0,
	Slot1 = 1,
	Slot2 = 2,
	Slot3 = 3,
	Slot4 = 4,
	Slot5 = 5,
	Slot6 = 6
};

struct REECHO_API FReEchoCsvIssue
{
	FString File;
	int32 Line = 0;
	FString Field;
	FString Message;

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoRuntimeSmokeEffectRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName RuntimeRowId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ParamName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Value = 0.0f;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoRuntimeSmokeRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayNameKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float TestScalar = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float TestPercent = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DistanceCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EffectKind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoCsvValueOp ModifierOp = EReEchoCsvValueOp::Override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FReEchoRuntimeSmokeEffectRow> Effects;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvCharacterRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SourceWorkbookId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName RoleId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 PromotionPriority = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName DefaultWeaponId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName AppearanceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FReEchoStatBlock BaseStats;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvCharacterAbilityRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName CharacterId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 Order = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Trigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EffectKind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Value = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Interval = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvCardEffectRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName CardId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 Order = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Trigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EffectKind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Value = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ParamName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ParamValue = 0.0f;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvElementRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SourceWorkbookId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoElement Element = EReEchoElement::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoElementRole Role = EReEchoElementRole::Attachment;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayNameKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString ColorHex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName VisualKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvStatusRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName StackPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName RefreshPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName MutexGroup;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FName> Tags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvReactionRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName TriggerElementId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName AttachmentElementId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName FormulaId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DamageMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DamageIncrease = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float RadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName StatusId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float StatusDurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float EnhancementMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bCanCrit = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bAffectedByEchoEfficiency = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bClearsAttachment = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvAttackStepRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName AttackPatternId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 StepIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DamageCoefficient = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float RangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ArcDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 ProjectileCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ConcentrationDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ExplosionRadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MovementCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bInvulnerable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName FormulaId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ConditionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float OuterRingStartFraction = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float OuterRingBonusMultiplier = 0.0f;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvWeaponTypeRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BaseAttackPatternId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SlotProfileId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float BaseIntervalSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float BaseRangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float BaseArcDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 BaseProjectileCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float BaseConcentrationDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float BaseExplosionRadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ChainWindowSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvWeaponRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName WeaponTypeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName VisualKey;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoInputSlot InputSlot = EReEchoInputSlot::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bStartSelectable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 LoadoutOrder = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName AttackPatternId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float AttackIntervalSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DamageCoefficient = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float RangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ArcDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 ProjectileCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ConcentrationDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ExplosionRadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 DataRevision = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvSlotTypeRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvSlotProfileRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName WeaponTypeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SlotTypeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SlotCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bRequired = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvPartEffectRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName PartId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 Order = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Trigger;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EffectKind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Value = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName FormulaId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName AttackPatternId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ParamName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ParamValue = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float CooldownSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName StackPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvPartRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName PartId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName WeaponTypeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SlotTypeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Rarity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FName> Tags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ReviewStatus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ImplementationStatus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bShopEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 ShopPrice = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FReEchoCsvPartEffectRow> Effects;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvShopPriceRangeRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName PriceCategory;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 MinPrice = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 MaxPrice = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvShopDropLevelRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 EncounterIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 FreeTier = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString ShopTiers; // pipe-delimited tiers, e.g. "2|3"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvShopRefreshRuleRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName RuleId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 CardSlotRefreshLimit = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 WeaponRuneRefreshLimit = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 CardSlotRefreshCost = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 WeaponRuneRefreshCost = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvCardRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SourceWorkbookId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 Tier = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Description;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FName> Tags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName PromotionRoleId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName OfferGroup;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bOfferable = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName StackPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ConflictPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ReviewStatus;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisabledReason;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FReEchoCsvCardEffectRow> Effects;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvEnemyAbilityRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName OwnerEnemyId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SequenceOrder = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Damage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float WindupSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ActiveSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float RecoverySeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float CooldownSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MinRangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MaxRangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float RadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float WidthCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float LengthCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ProjectileSpeedCmPerSecond = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float TeleportOffsetCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName TargetingMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName LockTiming;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float CleanseIntervalSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ImmunitySeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 ProjectileCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float SpreadAngleDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bMovementDuringCast = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvBossPhaseRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BossEnemyId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 PhaseIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float TriggerSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EchoPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float PhysicalAttackMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ElementalAttackMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float AttackSpeedMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MovementSpeedMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName RefillHealthPolicy;
	// WS4 (Plan 68): authoritative per-phase maximum health from the BossPhases worksheet. When > 0, the boss is
	// resized to this value when its phase advances (blood-bar depleted transition). It does NOT refill health —
	// first release forbids refill; this only sets the new ceiling for the incoming phase.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float PhaseMaxHealth = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvEnemyRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Archetype;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BehaviorProfileId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName PresentationId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MaxHealth = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MoveSpeedMultiplier = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float CollisionRadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float CollisionHalfHeightCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ContactDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float AttackIntervalSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ContactRangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MovementStopDistanceCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float HitReactionDurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float KnockbackSpeedCmPerSecond = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float KnockbackDrag = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float TriggerRadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DamageRadiusCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float FuseSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bBoss = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bPhase2Enabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Phase2TriggerRangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 Phase2RequiredAttackCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Phase2TransformSeconds = 0.0f;
	// WS4 (Plan 68): second-phase trigger model. "HealthThreshold" makes the boss transform when its health ratio
	// drops to/at HealthThresholdRatio (0 = depleted to zero). Empty/other keeps the legacy attack-count/range model.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Phase2TriggerMode;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float Phase2HealthThresholdRatio = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float HateRangeCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FReEchoCsvEnemyAbilityRow> Abilities;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	TArray<FReEchoCsvBossPhaseRow> BossPhases;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvEnemyShardDropRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 EncounterIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 MeleeMin = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 MeleeMax = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 RangedMin = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 RangedMax = 0;
	/** INDEX_NONE on both fields means that this encounter has no elite reward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 EliteMin = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 EliteMax = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvEnemyCombatStatRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EnemyId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 CombatIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MaxHealth = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float ContactDamage = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float AttackIntervalSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvStageRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 StageIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SceneId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 FirstEncounterIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 LastEncounterIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bPreserveEnemiesBetweenEncounters = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bClearEnemiesOnEnter = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvEncounterRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 EncounterIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName StageId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float DurationSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EndCondition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float EchoAnchorRatio = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float PlayerAnchorRatio = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName MeleeTargetingPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 RangedBurstLimit = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float RangedBurstWindowSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 EliteSkillConcurrency = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 ActiveUnitLimit = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bBossCountsTowardUnitLimit = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ReplayPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvEncounterWaveRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EncounterId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 WaveIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float TriggerSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 MeleeCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 RangedCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 EliteCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BossEnemyId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvSpawnProfileRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EnemyRole;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName EnemyId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MinAnchorDistanceCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MaxAnchorDistanceCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MinSpacingCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float WarningLeadSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName DistributionPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName SpacingPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvSpawnPolicyRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float AnchorLeadSeconds = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MinPlayerDistanceCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	float MinEchoDistanceCm = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName BoundaryPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName CandidatePolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName PlayerPredictionPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName MultiEchoPolicy;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 MaxCandidateAttempts = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString SourceSheet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 SourceRow = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Notes;
};

USTRUCT(BlueprintType)
struct REECHO_API FReEchoCsvAttributeRow
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 Tier = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString IconName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FName ValueKind;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	int32 DisplayOrder = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReEcho Data")
	FString Explanation;
};

struct REECHO_API FReEchoCsvDataSnapshot
{
	int32 SchemaVersion = 0;
	FString CardDomainRevision;
	FString WeaponDomainRevision;
	TMap<FName, FReEchoRuntimeSmokeRow> RuntimeSmokeRows;
	TMap<FName, FReEchoCsvCharacterRow> Characters;
	TMap<FName, FName> CharacterAliases;
	TMap<FName, FReEchoCsvCharacterAbilityRow> CharacterAbilities;
	TArray<FName> CharacterAbilityOrder;
	TMap<FName, FReEchoCsvCardRow> Cards;
	TArray<FName> CardOrder;
	TSharedPtr<const FReEchoCardCatalog> CardCatalog;
	TMap<FName, FReEchoCsvElementRow> Elements;
	TArray<FName> ElementOrder;
	TMap<FName, FReEchoCsvStatusRow> Statuses;
	TArray<FName> StatusOrder;
	TMap<FName, FReEchoCsvReactionRow> Reactions;
	TArray<FName> ReactionOrder;
	TMap<FName, FReEchoCsvWeaponTypeRow> WeaponTypes;
	TArray<FName> WeaponTypeOrder;
	TMap<FName, FReEchoCsvWeaponRow> Weapons;
	TArray<FName> WeaponOrder;
	TMap<FName, FReEchoCsvAttackStepRow> AttackSteps;
	TArray<FName> AttackStepOrder;
	TMap<FName, FReEchoCsvSlotTypeRow> SlotTypes;
	TMap<FName, FReEchoCsvSlotProfileRow> SlotProfiles;
	TMap<FName, FReEchoCsvPartRow> Parts;
	TMap<FName, FReEchoCsvEnemyRow> Enemies;
	TArray<FName> EnemyOrder;
	TMap<int32, FReEchoCsvEnemyShardDropRow> EnemyShardDrops;
	TMap<FName, FReEchoCsvEnemyCombatStatRow> EnemyCombatStats;
	TArray<FName> EnemyCombatStatOrder;
	TMap<FName, FReEchoCsvStageRow> Stages;
	TArray<FName> StageOrder;
	TMap<FName, FReEchoCsvEncounterRow> Encounters;
	TArray<FName> EncounterOrder;
	TMap<FName, FReEchoCsvEncounterWaveRow> EncounterWaves;
	TArray<FName> EncounterWaveOrder;
	TMap<FName, FReEchoCsvSpawnProfileRow> SpawnProfiles;
	TArray<FName> SpawnProfileOrder;
	TMap<FName, FReEchoCsvSpawnPolicyRow> SpawnPolicies;
	TMap<FName, FReEchoCsvAttributeRow> Attributes;
	TArray<FName> AttributeOrder;

	TMap<FName, FReEchoCsvShopPriceRangeRow> ShopPriceRanges;   // keyed by PriceCategory
	TMap<int32, FReEchoCsvShopDropLevelRow> ShopDropLevels;     // keyed by EncounterIndex (clamped to run length)
	TMap<FName, FReEchoCsvShopRefreshRuleRow> ShopRefreshRules; // keyed by RuleId; production uses Default

	const FReEchoRuntimeSmokeRow* FindRuntimeSmokeRow(FName RowId) const;
	FName ResolveCharacterId(FName CharacterId) const;
	const FReEchoCsvCharacterRow* FindCharacter(FName CharacterId) const;
	TArray<FReEchoCsvCharacterAbilityRow> GetCharacterAbilities(FName CharacterId, FName Trigger) const;
	const FReEchoCsvCardRow* FindCard(FName CardId) const;
	TArray<FReEchoCsvCardRow> GetOfferableCards(FName OfferGroup) const;
	const FReEchoCsvElementRow* FindElement(FName ElementId) const;
	const FReEchoCsvElementRow* FindElement(EReEchoElement Element) const;
	const FReEchoCsvStatusRow* FindStatus(FName StatusId) const;
	const FReEchoCsvReactionRow* FindReaction(FName TriggerElementId, FName AttachmentElementId) const;
	const FReEchoCsvWeaponTypeRow* FindWeaponType(FName WeaponTypeId) const;
	const FReEchoCsvWeaponRow* FindWeapon(FName WeaponId) const;
	const FReEchoCsvWeaponRow* FindEnabledWeapon(FName WeaponId) const;
	const FReEchoCsvWeaponRow* FindWeaponByInputSlot(EReEchoInputSlot InputSlot) const;
	TArray<FReEchoCsvWeaponRow> GetStartSelectableWeapons() const;
	TArray<FReEchoCsvAttackStepRow> GetAttackSteps(FName AttackPatternId) const;
	const FReEchoCsvEnemyRow* FindEnemy(FName EnemyId) const;
	const FReEchoCsvEnemyRow* FindEnabledEnemy(FName EnemyId) const;
	const FReEchoCsvEnemyShardDropRow* FindEnemyShardDrop(int32 EncounterIndex) const;
	const FReEchoCsvEnemyCombatStatRow* FindEnemyCombatStat(FName EnemyId, int32 CombatIndex) const;
	const FReEchoCsvStageRow* FindStage(FName StageId) const;
	const FReEchoCsvEncounterRow* FindEncounter(FName EncounterId) const;
	const FReEchoCsvEncounterRow* FindEncounterByIndex(int32 EncounterIndex) const;
	TArray<FReEchoCsvEncounterWaveRow> GetEncounterWaves(FName EncounterId) const;
	const FReEchoCsvSpawnProfileRow* FindSpawnProfileByRole(FName EnemyRole) const;
	const FReEchoCsvSpawnPolicyRow* FindEnabledSpawnPolicy() const;
	const FReEchoCsvAttributeRow* FindAttribute(FName AttributeId) const;
	const TArray<FName>& GetAttributeOrder() const;
};

struct REECHO_API FReEchoCsvLoadResult
{
	bool bSuccess = false;
	TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot;
	TArray<FReEchoCsvIssue> Issues;

	FString FormatIssues() const;
};

class REECHO_API FReEchoCsvDataRegistry
{
public:
	static constexpr int32 SupportedSchemaVersion = 1;

	static void RegisterBehaviorId(FName BehaviorId);
	static void RegisterEffectKind(FName EffectKind);
	static void RegisterFormulaId(FName FormulaId);
	static void RegisterAttackPatternId(FName AttackPatternId);
	static void RegisterBuiltInCsvBehaviors();
	static bool IsBehaviorIdRegistered(FName BehaviorId);
	static bool IsEffectKindRegistered(FName EffectKind);
	static bool IsFormulaIdRegistered(FName FormulaId);
	static bool IsAttackPatternIdRegistered(FName AttackPatternId);

	static FString GetDefaultDataCatalogPath();
	static FReEchoCsvLoadResult LoadSnapshotFromCatalog(const UReEchoGameDataCatalog& Catalog);
	static FReEchoCsvLoadResult LoadAndPublishFromCatalog(const UReEchoGameDataCatalog& Catalog);
	static FReEchoCsvLoadResult LoadAndPublishDefault();
	static TSharedPtr<const FReEchoCsvDataSnapshot> GetSnapshot();
	static void ClearPublishedSnapshotForTests();

private:
	static void EnsureDefaultRegistrations();
};
