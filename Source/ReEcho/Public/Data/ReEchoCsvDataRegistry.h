#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"

enum class EReEchoCsvValueOp : uint8
{
	Add,
	Multiply,
	Override
};

enum class EReEchoElementRole : uint8
{
	Trigger,
	Attachment
};

enum class EReEchoInputSlot : uint8
{
	None = 0,
	Slot1 = 1,
	Slot2 = 2,
	Slot3 = 3
};

struct REECHO_API FReEchoCsvIssue
{
	FString File;
	int32 Line = 0;
	FString Field;
	FString Message;

	FString ToString() const;
};

struct REECHO_API FReEchoRuntimeSmokeEffectRow
{
	FName Id;
	FName RuntimeRowId;
	FName ParamName;
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	float Value = 0.0f;
};

struct REECHO_API FReEchoRuntimeSmokeRow
{
	FName Id;
	FString DisplayNameKey;
	bool bEnabled = false;
	float TestScalar = 0.0f;
	float TestPercent = 0.0f;
	float DistanceCm = 0.0f;
	float DurationSeconds = 0.0f;
	FName BehaviorId;
	FName EffectKind;
	EReEchoCsvValueOp ModifierOp = EReEchoCsvValueOp::Override;
	TArray<FReEchoRuntimeSmokeEffectRow> Effects;
};

struct REECHO_API FReEchoCsvCharacterRow
{
	FName Id;
	FName SourceWorkbookId;
	FString DisplayName;
	bool bEnabled = false;
	FString DisabledReason;
	FName RoleId;
	int32 PromotionPriority = 0;
	FName DefaultWeaponId;
	FName AppearanceId;
	FName PassiveBehaviorId;
	float PassiveValue = 0.0f;
	FReEchoStatBlock BaseStats;
};

struct REECHO_API FReEchoCsvCardEffectRow
{
	FName Id;
	FName CardId;
	int32 Order = 0;
	FName Trigger;
	FName EffectKind;
	FName Target;
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	float Value = 0.0f;
	FName BehaviorId;
	FName ParamName;
	float ParamValue = 0.0f;
};

struct REECHO_API FReEchoCsvElementRow
{
	FName Id;
	FName SourceWorkbookId;
	EReEchoElement Element = EReEchoElement::None;
	EReEchoElementRole Role = EReEchoElementRole::Attachment;
	FString DisplayName;
	FString DisplayNameKey;
	FString ColorHex;
	FName VisualKey;
	bool bEnabled = false;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvStatusRow
{
	FName Id;
	FString DisplayName;
	FName BehaviorId;
	float DurationSeconds = 0.0f;
	FName StackPolicy;
	FName RefreshPolicy;
	FName MutexGroup;
	TArray<FName> Tags;
	bool bEnabled = false;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvReactionRow
{
	FName Id;
	FString DisplayName;
	FName TriggerElementId;
	FName AttachmentElementId;
	FName BehaviorId;
	FName FormulaId;
	float DamageMultiplier = 1.0f;
	float DamageIncrease = 0.0f;
	float RadiusCm = 0.0f;
	FName StatusId;
	float StatusDurationSeconds = 0.0f;
	float EnhancementMultiplier = 1.0f;
	bool bCanCrit = false;
	bool bAffectedByEchoEfficiency = false;
	bool bClearsAttachment = true;
	bool bEnabled = false;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvAttackStepRow
{
	FName Id;
	FName AttackPatternId;
	int32 StepIndex = 0;
	float DurationSeconds = 0.0f;
	float PhysicalCoefficient = 0.0f;
	float ElementalCoefficient = 0.0f;
	float RangeCm = 0.0f;
	float ArcDegrees = 0.0f;
	int32 ProjectileCount = 0;
	float ConcentrationDegrees = 0.0f;
	float ExplosionRadiusCm = 0.0f;
	float MovementCm = 0.0f;
	bool bInvulnerable = false;
	FName BehaviorId;
	FName FormulaId;
	FName ConditionId;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvWeaponTypeRow
{
	FName Id;
	FString DisplayName;
	FName BaseAttackPatternId;
	FName SlotProfileId;
	float BaseIntervalSeconds = 0.0f;
	float BaseRangeCm = 0.0f;
	float BaseArcDegrees = 0.0f;
	int32 BaseProjectileCount = 0;
	float BaseConcentrationDegrees = 0.0f;
	float BaseExplosionRadiusCm = 0.0f;
	float ChainWindowSeconds = 0.0f;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvWeaponRow
{
	FName Id;
	FName WeaponTypeId;
	FString DisplayName;
	FName VisualKey;
	EReEchoInputSlot InputSlot = EReEchoInputSlot::None;
	bool bStartSelectable = false;
	int32 LoadoutOrder = 0;
	FName AttackPatternId;
	float AttackIntervalSeconds = 0.0f;
	float PhysicalCoefficient = 0.0f;
	float ElementalCoefficient = 0.0f;
	float RangeCm = 0.0f;
	float ArcDegrees = 0.0f;
	int32 ProjectileCount = 0;
	float ConcentrationDegrees = 0.0f;
	float ExplosionRadiusCm = 0.0f;
	int32 DataRevision = 0;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvSlotTypeRow
{
	FName Id;
	FString DisplayName;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvSlotProfileRow
{
	FName Id;
	FName WeaponTypeId;
	FName SlotTypeId;
	int32 SlotCount = 0;
	bool bRequired = false;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvPartEffectRow
{
	FName Id;
	FName PartId;
	int32 Order = 0;
	FName Trigger;
	FName EffectKind;
	FName Target;
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	float Value = 0.0f;
	FName BehaviorId;
	FName FormulaId;
	FName AttackPatternId;
	FName ParamName;
	float ParamValue = 0.0f;
	float DurationSeconds = 0.0f;
	float CooldownSeconds = 0.0f;
	FName StackPolicy;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvPartRow
{
	FName Id;
	FName PartId;
	FName WeaponTypeId;
	FName SlotTypeId;
	FString DisplayName;
	FString Description;
	FName Rarity;
	TArray<FName> Tags;
	bool bEnabled = false;
	FName ReviewStatus;
	FName ImplementationStatus;
	FString DisabledReason;
	FString SourceSheet;
	int32 SourceRow = 0;
	TArray<FReEchoCsvPartEffectRow> Effects;
};

struct REECHO_API FReEchoCsvCardRow
{
	FName Id;
	FName SourceWorkbookId;
	int32 Tier = 0;
	FString DisplayName;
	FString Description;
	TArray<FName> Tags;
	FName PromotionRoleId;
	FName OfferGroup;
	bool bEnabled = false;
	bool bOfferable = false;
	FName StackPolicy;
	FName ConflictPolicy;
	FName ReviewStatus;
	FString DisabledReason;
	TArray<FReEchoCsvCardEffectRow> Effects;
};

struct REECHO_API FReEchoCsvDataSnapshot
{
	int32 SchemaVersion = 0;
	FString WeaponDomainRevision;
	TMap<FName, FReEchoRuntimeSmokeRow> RuntimeSmokeRows;
	TMap<FName, FReEchoCsvCharacterRow> Characters;
	TMap<FName, FName> CharacterAliases;
	TMap<FName, FReEchoCsvCardRow> Cards;
	TArray<FName> CardOrder;
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

	const FReEchoRuntimeSmokeRow* FindRuntimeSmokeRow(FName RowId) const;
	FName ResolveCharacterId(FName CharacterId) const;
	const FReEchoCsvCharacterRow* FindCharacter(FName CharacterId) const;
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

	static FString GetDefaultDataDirectory();
	static FReEchoCsvLoadResult LoadSnapshotFromDirectory(const FString& DataDirectory);
	static FReEchoCsvLoadResult LoadAndPublishFromDirectory(const FString& DataDirectory);
	static FReEchoCsvLoadResult LoadAndPublishDefault();
	static TSharedPtr<const FReEchoCsvDataSnapshot> GetSnapshot();
	static void ClearPublishedSnapshotForTests();

private:
	static void EnsureDefaultRegistrations();
};
