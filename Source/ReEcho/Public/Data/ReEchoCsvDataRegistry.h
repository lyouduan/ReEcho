#pragma once

#include "CoreMinimal.h"
#include "Cards/ReEchoCardCatalog.h"
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
	FString Description;
	bool bEnabled = false;
	FString DisabledReason;
	FName RoleId;
	int32 PromotionPriority = 0;
	FName DefaultWeaponId;
	FName AppearanceId;
	FReEchoStatBlock BaseStats;
};

struct REECHO_API FReEchoCsvCharacterAbilityRow
{
	FName Id;
	FName CharacterId;
	int32 Order = 0;
	FName Trigger;
	FName EffectKind;
	FName Target;
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	float Value = 0.0f;
	FName BehaviorId;
	float Interval = 0.0f;
	/**
	 * Card-pack tier required for this cadence ability's counter to advance. 0 counts every card pack;
	 * a positive value only counts packs at or above that tier (e.g. 2 = non-tier-1 packs only).
	 */
	int32 MinCardPackTier = 0;
	bool bEnabled = false;
	FString DisabledReason;
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
	FString Description;
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
	FString Description;
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
	FString Description;
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
	float DamageCoefficient = 0.0f;
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
	float OuterRingStartFraction = 0.0f;
	float OuterRingBonusMultiplier = 0.0f;
};

struct REECHO_API FReEchoCsvWeaponTypeRow
{
	FName Id;
	FString DisplayName;
	FString Description;
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
	float DamageCoefficient = 0.0f;
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
	bool bShopEnabled = false;
	int32 ShopPrice = 0;
	FString DisabledReason;
	FString SourceSheet;
	int32 SourceRow = 0;
	TArray<FReEchoCsvPartEffectRow> Effects;
};

struct REECHO_API FReEchoCsvShopPriceRangeRow
{
	FName PriceCategory;
	int32 MinPrice = 0;
	int32 MaxPrice = 0;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvShopDropLevelRow
{
	int32 EncounterIndex = 0;
	int32 FreeTier = 0;
	FString ShopTiers; // pipe-delimited tier:count pairs, e.g. "1:3|2:1|3:1" (count defaults to 1)
	FString SourceSheet;
	int32 SourceRow = 0;
	FString DisabledReason;
};

struct REECHO_API FReEchoCsvRuneUpgradeRow
{
	FName FromPartId = NAME_None; // lower-tier rune consumed by the recipe
	int32 NeedCount = 0;          // how many lower-tier runes are required
	FName ToPartId = NAME_None;   // higher-tier rune produced by the recipe
	FString SourceSheet;
	int32 SourceRow = 0;
};

struct REECHO_API FReEchoCsvShopRefreshRuleRow
{
	FName RuleId;
	int32 CardSlotRefreshLimit = 0;
	int32 WeaponRuneRefreshLimit = 0;
	int32 CardSlotRefreshCost = 0;
	int32 WeaponRuneRefreshCost = 0;
	FString SourceSheet;
	int32 SourceRow = 0;
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

struct REECHO_API FReEchoCsvEnemyAbilityRow
{
	FName Id;
	FName OwnerEnemyId;
	FName BehaviorId;
	int32 SequenceOrder = 0;
	bool bEnabled = false;
	float Damage = 0.0f;
	float WindupSeconds = 0.0f;
	float ActiveSeconds = 0.0f;
	float RecoverySeconds = 0.0f;
	float CooldownSeconds = 0.0f;
	float MinRangeCm = 0.0f;
	float MaxRangeCm = 0.0f;
	float RadiusCm = 0.0f;
	float WidthCm = 0.0f;
	float LengthCm = 0.0f;
	float ProjectileSpeedCmPerSecond = 0.0f;
	float TeleportOffsetCm = 0.0f;
	FName TargetingMode;
	FName LockTiming;
	float CleanseIntervalSeconds = 0.0f;
	float ImmunitySeconds = 0.0f;
	int32 ProjectileCount = 1;
	float SpreadAngleDegrees = 0.0f;
	bool bMovementDuringCast = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvBossPhaseRow
{
	FName Id;
	FName BossEnemyId;
	int32 PhaseIndex = 0;
	float TriggerSeconds = 0.0f;
	FName EchoPolicy;
	float PhysicalAttackMultiplier = 1.0f;
	float ElementalAttackMultiplier = 1.0f;
	float AttackSpeedMultiplier = 1.0f;
	float MovementSpeedMultiplier = 1.0f;
	FName RefillHealthPolicy;
	// WS4 (Plan 68): authoritative per-phase maximum health from the BossPhases worksheet. When > 0, the boss is
	// resized to this value when its phase advances (blood-bar depleted transition). It does NOT refill health —
	// first release forbids refill; this only sets the new ceiling for the incoming phase.
	float PhaseMaxHealth = 0.0f;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvEnemyRow
{
	FName Id;
	FName Archetype;
	FName BehaviorProfileId;
	FName PresentationId;
	bool bEnabled = false;
	float MaxHealth = 0.0f;
	float MoveSpeedMultiplier = 0.0f;
	float CollisionRadiusCm = 0.0f;
	float CollisionHalfHeightCm = 0.0f;
	float ContactDamage = 0.0f;
	float AttackIntervalSeconds = 0.0f;
	float ContactRangeCm = 0.0f;
	float MovementStopDistanceCm = 0.0f;
	float HitReactionDurationSeconds = 0.0f;
	float KnockbackSpeedCmPerSecond = 0.0f;
	float KnockbackDrag = 0.0f;
	float TriggerRadiusCm = 0.0f;
	float DamageRadiusCm = 0.0f;
	float FuseSeconds = 0.0f;
	bool bBoss = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
	bool bPhase2Enabled = false;
	float Phase2TriggerRangeCm = 0.0f;
	int32 Phase2RequiredAttackCount = 0;
	float Phase2TransformSeconds = 0.0f;
	// WS4 (Plan 68): second-phase trigger model. "HealthThreshold" makes the boss transform when its health ratio
	// drops to/at HealthThresholdRatio (0 = depleted to zero). Empty/other keeps the legacy attack-count/range model.
	FString Phase2TriggerMode;
	float Phase2HealthThresholdRatio = 0.0f;
	float HateRangeCm = 0.0f;
	TArray<FReEchoCsvEnemyAbilityRow> Abilities;
	TArray<FReEchoCsvBossPhaseRow> BossPhases;
};

struct REECHO_API FReEchoCsvEnemyShardDropRow
{
	int32 EncounterIndex = 0;
	int32 MeleeMin = 0;
	int32 MeleeMax = 0;
	int32 RangedMin = 0;
	int32 RangedMax = 0;
	/** INDEX_NONE on both fields means that this encounter has no elite reward. */
	int32 EliteMin = INDEX_NONE;
	int32 EliteMax = INDEX_NONE;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvEnemyCombatStatRow
{
	FName Id;
	FName EnemyId;
	int32 CombatIndex = 0;
	float MaxHealth = 0.0f;
	float ContactDamage = 0.0f;
	float AttackIntervalSeconds = 0.0f;
};

struct REECHO_API FReEchoCsvStageRow
{
	FName Id;
	int32 StageIndex = 0;
	FName SceneId;
	int32 FirstEncounterIndex = 0;
	int32 LastEncounterIndex = 0;
	bool bPreserveEnemiesBetweenEncounters = false;
	bool bClearEnemiesOnEnter = true;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvEncounterRow
{
	FName Id;
	int32 EncounterIndex = 0;
	FName StageId;
	float DurationSeconds = 0.0f;
	FName EndCondition;
	float EchoAnchorRatio = 0.0f;
	float PlayerAnchorRatio = 1.0f;
	FName MeleeTargetingPolicy;
	int32 RangedBurstLimit = 0;
	float RangedBurstWindowSeconds = 0.0f;
	int32 EliteSkillConcurrency = 0;
	int32 ActiveUnitLimit = 0;
	bool bBossCountsTowardUnitLimit = true;
	FName ReplayPolicy;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvEncounterWaveRow
{
	FName Id;
	FName EncounterId;
	int32 WaveIndex = 0;
	float TriggerSeconds = 0.0f;
	int32 MeleeCount = 0;
	int32 RangedCount = 0;
	int32 EliteCount = 0;
	FName BossEnemyId;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvSpawnProfileRow
{
	FName Id;
	FName EnemyRole;
	FName EnemyId;
	float MinAnchorDistanceCm = 0.0f;
	float MaxAnchorDistanceCm = 0.0f;
	float MinSpacingCm = 0.0f;
	float WarningLeadSeconds = 0.0f;
	FName DistributionPolicy;
	FName SpacingPolicy;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvSpawnPolicyRow
{
	FName Id;
	float AnchorLeadSeconds = 0.0f;
	float MinPlayerDistanceCm = 0.0f;
	float MinEchoDistanceCm = 0.0f;
	FName BoundaryPolicy;
	FName CandidatePolicy;
	FName PlayerPredictionPolicy;
	FName MultiEchoPolicy;
	int32 MaxCandidateAttempts = 0;
	bool bEnabled = false;
	FString SourceSheet;
	int32 SourceRow = 0;
	FString Notes;
};

struct REECHO_API FReEchoCsvAttributeRow
{
	FName Id;
	FString DisplayName;
	int32 Tier = 0;
	FString IconName;
	FName ValueKind;
	int32 DisplayOrder = 0;
	FString Explanation;
};

struct REECHO_API FReEchoCsvDataSnapshot
{
	int32 SchemaVersion = 0;
	/** Standard on the boot snapshot; overwritten on immutable run-difficulty overlays. */
	EReEchoRunDifficulty Difficulty = EReEchoRunDifficulty::Standard;
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
	TMap<FName, FReEchoCsvRuneUpgradeRow> RuneUpgrades;          // keyed by FromPartId; I->II->III synthesis recipes

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

	static FString GetDefaultDataDirectory();
	static FReEchoCsvLoadResult LoadSnapshotFromDirectory(const FString& DataDirectory);
	static FReEchoCsvLoadResult LoadAndPublishFromDirectory(const FString& DataDirectory);
	static FReEchoCsvLoadResult LoadAndPublishDefault();
	/** Loads and caches one complete run-difficulty overlay. Never silently substitutes another difficulty. */
	static FReEchoCsvLoadResult LoadDifficultySnapshot(EReEchoRunDifficulty Difficulty);
	static TSharedPtr<const FReEchoCsvDataSnapshot> GetSnapshot();
	static void ClearPublishedSnapshotForTests();

private:
	static void EnsureDefaultRegistrations();
};
