#include "Data/ReEchoCsvDataRegistry.h"

#include "Cards/ReEchoCardCatalog.h"
#include "Combat/ReEchoElementRuntime.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "ReEcho.h"
#include "ReEchoCharacterBuildCsvReader.h"
#include "ReEchoCsvDataReader.h"
#include "ReEchoElementReactionCsvReader.h"
#include "ReEchoEncounterCsvReader.h"
#include "ReEchoEnemyCsvReader.h"
#include "ReEchoWeaponCsvReader.h"

namespace
{
constexpr const TCHAR* RuntimeSmokeTableId = TEXT("RuntimeSmoke");
constexpr const TCHAR* RuntimeSmokeEffectsTableId = TEXT("RuntimeSmokeEffects");
constexpr const TCHAR* CharactersTableId = TEXT("Characters");
constexpr const TCHAR* CharacterAliasesTableId = TEXT("CharacterAliases");
constexpr const TCHAR* CharacterAbilitiesTableId = TEXT("CharacterAbilities");
constexpr const TCHAR* CardsTableId = TEXT("Cards");
constexpr const TCHAR* CardEffectsTableId = TEXT("CardEffects");
constexpr const TCHAR* ElementsTableId = TEXT("Elements");
constexpr const TCHAR* StatusesTableId = TEXT("Statuses");
constexpr const TCHAR* ReactionsTableId = TEXT("Reactions");
constexpr const TCHAR* WeaponTypesTableId = TEXT("WeaponTypes");
constexpr const TCHAR* WeaponsTableId = TEXT("Weapons");
constexpr const TCHAR* AttackStepsTableId = TEXT("AttackSteps");
constexpr const TCHAR* SlotTypesTableId = TEXT("SlotTypes");
constexpr const TCHAR* SlotProfilesTableId = TEXT("SlotProfiles");
constexpr const TCHAR* PartsTableId = TEXT("Parts");
constexpr const TCHAR* PartEffectsTableId = TEXT("PartEffects");
constexpr const TCHAR* ShopPriceRangesTableId = TEXT("shop_price_ranges");
constexpr const TCHAR* ShopDropLevelsTableId = TEXT("shop_drop_levels");
constexpr const TCHAR* ShopRefreshRulesTableId = TEXT("shop_refresh_rules");
constexpr const TCHAR* EnemiesTableId = TEXT("Enemies");
constexpr const TCHAR* EnemyAbilitiesTableId = TEXT("EnemyAbilities");
constexpr const TCHAR* BossPhasesTableId = TEXT("BossPhases");
constexpr const TCHAR* EnemyCombatStatsTableId = TEXT("EnemyCombatStats");
constexpr const TCHAR* EnemyShardDropsTableId = TEXT("EnemyShardDrops");
constexpr const TCHAR* StagesTableId = TEXT("Stages");
constexpr const TCHAR* EncountersTableId = TEXT("Encounters");
constexpr const TCHAR* EncounterWavesTableId = TEXT("EncounterWaves");
constexpr const TCHAR* SpawnProfilesTableId = TEXT("SpawnProfiles");
constexpr const TCHAR* SpawnPolicyTableId = TEXT("SpawnPolicy");
constexpr const TCHAR* AttributesTableId = TEXT("Attributes");

constexpr const TCHAR* BehaviorNone = TEXT("None");
constexpr const TCHAR* DefaultBehaviorId = TEXT("RuntimeSmoke.LogValue");
constexpr const TCHAR* DefaultEffectKind = TEXT("ScalarModifier");
constexpr const TCHAR* StatModifierEffectKind = TEXT("StatModifier");
constexpr const TCHAR* InstantRecoveryEffectKind = TEXT("InstantRecovery");
constexpr const TCHAR* CardBehaviorEffectKind = TEXT("CardBehavior");
constexpr const TCHAR* ElementReactionEffectKind = TEXT("ElementReaction");

FCriticalSection RegistryCriticalSection;
TSharedPtr<const FReEchoCsvDataSnapshot> PublishedSnapshot;
TSet<FName> RegisteredBehaviorIds;
TSet<FName> RegisteredEffectKinds;
TSet<FName> RegisteredFormulaIds;
TSet<FName> RegisteredAttackPatternIds;
bool bDefaultRegistrationsReady = false;

EReEchoCardValueOperation ToCardValueOperation(const EReEchoCsvValueOp Operation)
{
	switch (Operation)
	{
		case EReEchoCsvValueOp::Add:
			return EReEchoCardValueOperation::Add;
		case EReEchoCsvValueOp::Multiply:
			return EReEchoCardValueOperation::Multiply;
		case EReEchoCsvValueOp::Override:
			return EReEchoCardValueOperation::Override;
		default:
			return EReEchoCardValueOperation::Add;
	}
}

bool CompileCardCatalog(const FString& DataDirectory,
                        const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                        FReEchoCsvDataSnapshot& Snapshot,
                        TArray<FReEchoCsvIssue>& Issues)
{
	TArray<uint8> RevisionBytes;
	for (const TCHAR* TableId : {CardsTableId, CardEffectsTableId})
	{
		TArray<uint8> TableBytes;
		const FString Path = FPaths::Combine(DataDirectory, ManifestEntries[TableId].FileName);
		if (!FFileHelper::LoadFileToArray(TableBytes, *Path))
		{
			ReEchoCsv::AddIssue(Issues, Path, 1, TEXT("Revision"), TEXT("Cannot read card domain bytes"));
			return false;
		}
		RevisionBytes.Append(TableBytes);
	}
	Snapshot.CardDomainRevision =
	    FString::Printf(TEXT("cards-v1-%08x"), FCrc::MemCrc32(RevisionBytes.GetData(), RevisionBytes.Num()));

	TArray<FReEchoCardDefinition> Definitions;
	for (const FName CardId : Snapshot.CardOrder)
	{
		const FReEchoCsvCardRow* Row = Snapshot.Cards.Find(CardId);
		if (!Row)
		{
			continue;
		}
		FReEchoCardDefinition Definition;
		Definition.Id = Row->Id;
		Definition.Tier = Row->Tier;
		Definition.DisplayName = Row->DisplayName;
		Definition.Description = Row->Description;
		Definition.Tags = Row->Tags;
		Definition.PromotionRoleId = Row->PromotionRoleId;
		Definition.OfferGroup = Row->OfferGroup;
		Definition.StackPolicy = Row->StackPolicy;
		Definition.ConflictPolicy = Row->ConflictPolicy;
		Definition.bEnabled = Row->bEnabled;
		Definition.bOfferable = Row->bOfferable;
		for (const FReEchoCsvCardEffectRow& EffectRow : Row->Effects)
		{
			FReEchoCardEffectDefinition Effect;
			Effect.Id = EffectRow.Id;
			Effect.Order = EffectRow.Order;
			Effect.Trigger = EffectRow.Trigger;
			Effect.BehaviorId = EffectRow.BehaviorId;
			Effect.Target = EffectRow.Target;
			Effect.Operation = ToCardValueOperation(EffectRow.ValueOp);
			Effect.Value = EffectRow.Value;
			Effect.ParamName = EffectRow.ParamName;
			Effect.ParamValue = EffectRow.ParamValue;
			Definition.Effects.Add(MoveTemp(Effect));
		}
		Definitions.Add(MoveTemp(Definition));
	}

	TSharedRef<FReEchoCardCatalog> Catalog = MakeShared<FReEchoCardCatalog>();
	FString Error;
	if (!Catalog->Initialize(Definitions, Snapshot.CardDomainRevision, Error))
	{
		ReEchoCsv::AddIssue(Issues,
		                    FPaths::Combine(DataDirectory, ManifestEntries[CardsTableId].FileName),
		                    1,
		                    TEXT("CardCatalog"),
		                    Error);
		return false;
	}
	Snapshot.CardCatalog = Catalog;
	return true;
}

TSharedRef<const FReEchoElementRuleSet> CompileElementRuleSet(const FReEchoCsvDataSnapshot& Snapshot)
{
	TSharedRef<FReEchoElementRuleSet> Rules = MakeShared<FReEchoElementRuleSet>();
	for (const TPair<FName, FReEchoCsvElementRow>& Pair : Snapshot.Elements)
	{
		const FReEchoCsvElementRow& Row = Pair.Value;
		FReEchoElementRuleDefinition Definition;
		Definition.ElementId = Row.Id;
		Definition.Element = Row.Element;
		Definition.bAttachment = Row.Role == EReEchoElementRole::Attachment;
		Definition.bEnabled = Row.bEnabled;
		Rules->Elements.Add(Row.Element, MoveTemp(Definition));
	}
	for (const TPair<FName, FReEchoCsvStatusRow>& Pair : Snapshot.Statuses)
	{
		const FReEchoCsvStatusRow& Row = Pair.Value;
		FReEchoStatusRuleDefinition Definition;
		Definition.StatusId = Row.Id;
		Definition.DurationSeconds = Row.DurationSeconds;
		Definition.bEnabled = Row.bEnabled;
		Rules->Statuses.Add(Row.Id, MoveTemp(Definition));
	}
	for (const TPair<FName, FReEchoCsvReactionRow>& Pair : Snapshot.Reactions)
	{
		const FReEchoCsvReactionRow& Row = Pair.Value;
		FReEchoReactionRuleDefinition Definition;
		Definition.ReactionId = Row.Id;
		Definition.TriggerElementId = Row.TriggerElementId;
		Definition.AttachmentElementId = Row.AttachmentElementId;
		Definition.BehaviorId = Row.BehaviorId;
		Definition.FormulaId = Row.FormulaId;
		Definition.DamageMultiplier = Row.DamageMultiplier;
		Definition.DamageIncrease = Row.DamageIncrease;
		Definition.RadiusCm = Row.RadiusCm;
		Definition.StatusId = Row.StatusId;
		Definition.StatusDurationSeconds = Row.StatusDurationSeconds;
		Definition.EnhancementMultiplier = Row.EnhancementMultiplier;
		Definition.bCanCrit = Row.bCanCrit;
		Definition.bAffectedByEchoEfficiency = Row.bAffectedByEchoEfficiency;
		Definition.bClearsAttachment = Row.bClearsAttachment;
		Definition.bEnabled = Row.bEnabled;
		Rules->Reactions.Add(Row.Id, MoveTemp(Definition));
	}
	return Rules;
}

TArray<FString> GetRequiredTableIds()
{
	return {RuntimeSmokeTableId,
	        RuntimeSmokeEffectsTableId,
	        CharactersTableId,
	        CharacterAliasesTableId,
	        CharacterAbilitiesTableId,
	        CardsTableId,
	        CardEffectsTableId,
	        ElementsTableId,
	        StatusesTableId,
	        ReactionsTableId,
	        WeaponTypesTableId,
	        WeaponsTableId,
	        AttackStepsTableId,
	        SlotTypesTableId,
	        SlotProfilesTableId,
	        PartsTableId,
	        PartEffectsTableId,
	        ShopPriceRangesTableId,
	        ShopDropLevelsTableId,
	        ShopRefreshRulesTableId,
	        EnemiesTableId,
	        EnemyAbilitiesTableId,
	        BossPhasesTableId,
	        EnemyCombatStatsTableId,
	        EnemyShardDropsTableId,
	        StagesTableId,
	        EncountersTableId,
	        EncounterWavesTableId,
	        SpawnProfilesTableId,
	        SpawnPolicyTableId,
	        AttributesTableId};
}

bool ReadRuntimeSmokeTable(const FString& DataDirectory,
                           const ReEchoCsv::FManifestEntry& Entry,
                           FReEchoCsvDataSnapshot& Snapshot,
                           TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("DisplayNameKey"),
	                            TEXT("Enabled"),
	                            TEXT("TestScalar"),
	                            TEXT("TestPercent"),
	                            TEXT("DistanceCm"),
	                            TEXT("DurationSeconds"),
	                            TEXT("BehaviorId"),
	                            TEXT("EffectKind"),
	                            TEXT("ModifierOp")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoRuntimeSmokeRow RuntimeRow;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), RuntimeRow.Id, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayNameKey"), RuntimeRow.DisplayNameKey, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), RuntimeRow.bEnabled, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("TestScalar"), 0.0f, 100000.0f, RuntimeRow.TestScalar, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("TestPercent"), 0.0f, 1.0f, RuntimeRow.TestPercent, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DistanceCm"), 0.0f, 1000000.0f, RuntimeRow.DistanceCm, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DurationSeconds"), 0.0f, 3600.0f, RuntimeRow.DurationSeconds, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BehaviorId"), RuntimeRow.BehaviorId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("EffectKind"), RuntimeRow.EffectKind, Issues);
		ReEchoCsv::RequireValueOp(Table, Row, TEXT("ModifierOp"), RuntimeRow.ModifierOp, Issues);

		if (RuntimeRow.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(RuntimeRow.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(RuntimeRow.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!FReEchoCsvDataRegistry::IsEffectKindRegistered(RuntimeRow.EffectKind))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("EffectKind"), TEXT("Unknown registered C++ effect kind"));
		}
		SeenIds.Add(RuntimeRow.Id);
		Snapshot.RuntimeSmokeRows.Add(RuntimeRow.Id, RuntimeRow);
	}
	return Issues.Num() == 0;
}

bool ReadAttributesTable(const FString& DataDirectory,
                         const ReEchoCsv::FManifestEntry& Entry,
                         FReEchoCsvDataSnapshot& Snapshot,
                         TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	ReEchoCsv::HasExactColumns(Table,
	                           {TEXT("Id"),
	                            TEXT("DisplayName"),
	                            TEXT("Tier"),
	                            TEXT("IconName"),
	                            TEXT("ValueKind"),
	                            TEXT("DisplayOrder"),
	                            TEXT("Explanation")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvAttributeRow Attr;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Attr.Id, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Attr.DisplayName, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("Tier"), Attr.Tier, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("IconName"), Attr.IconName, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ValueKind"), Attr.ValueKind, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("DisplayOrder"), Attr.DisplayOrder, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("Explanation"), Attr.Explanation);

		if (Attr.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Attr.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		SeenIds.Add(Attr.Id);
		Snapshot.Attributes.Add(Attr.Id, Attr);
		Snapshot.AttributeOrder.Add(Attr.Id);
	}

	Snapshot.AttributeOrder.Sort(
	    [&Snapshot](const FName& A, const FName& B)
	    {
		    const FReEchoCsvAttributeRow* RA = Snapshot.Attributes.Find(A);
		    const FReEchoCsvAttributeRow* RB = Snapshot.Attributes.Find(B);
		    const int32 OA = RA ? RA->DisplayOrder : 0;
		    const int32 OB = RB ? RB->DisplayOrder : 0;
		    return OA < OB;
	    });

	return Issues.Num() == 0;
}

bool ReadRuntimeSmokeEffectsTable(const FString& DataDirectory,
                                  const ReEchoCsv::FManifestEntry& Entry,
                                  FReEchoCsvDataSnapshot& Snapshot,
                                  TArray<FReEchoCsvIssue>& Issues)
{
	ReEchoCsv::FTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ReEchoCsv::ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	ReEchoCsv::HasExactColumns(
	    Table, {TEXT("Id"), TEXT("RuntimeRowId"), TEXT("ParamName"), TEXT("ValueOp"), TEXT("Value")}, Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoRuntimeSmokeEffectRow EffectRow;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), EffectRow.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("RuntimeRowId"), EffectRow.RuntimeRowId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ParamName"), EffectRow.ParamName, Issues);
		ReEchoCsv::RequireValueOp(Table, Row, TEXT("ValueOp"), EffectRow.ValueOp, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("Value"), -100000.0f, 100000.0f, EffectRow.Value, Issues);

		if (EffectRow.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(EffectRow.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (FReEchoRuntimeSmokeRow* Parent = Snapshot.RuntimeSmokeRows.Find(EffectRow.RuntimeRowId))
		{
			Parent->Effects.Add(EffectRow);
		}
		else
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("RuntimeRowId"), TEXT("Unknown RuntimeSmoke.Id reference"));
		}
		SeenIds.Add(EffectRow.Id);
	}
	return Issues.Num() == 0;
}

}

FString FReEchoCsvIssue::ToString() const
{
	return FString::Printf(TEXT("%s:%d:%s: %s"), *File, Line, *Field, *Message);
}

const FReEchoRuntimeSmokeRow* FReEchoCsvDataSnapshot::FindRuntimeSmokeRow(const FName RowId) const
{
	return RuntimeSmokeRows.Find(RowId);
}

FName FReEchoCsvDataSnapshot::ResolveCharacterId(const FName CharacterId) const
{
	if (const FName* CanonicalId = CharacterAliases.Find(CharacterId))
	{
		return *CanonicalId;
	}
	return CharacterId;
}

const FReEchoCsvCharacterRow* FReEchoCsvDataSnapshot::FindCharacter(const FName CharacterId) const
{
	return Characters.Find(ResolveCharacterId(CharacterId));
}

TArray<FReEchoCsvCharacterAbilityRow> FReEchoCsvDataSnapshot::GetCharacterAbilities(const FName CharacterId,
                                                                                    const FName Trigger) const
{
	TArray<FReEchoCsvCharacterAbilityRow> Result;
	const FName CanonicalId = ResolveCharacterId(CharacterId);
	for (const FName AbilityId : CharacterAbilityOrder)
	{
		const FReEchoCsvCharacterAbilityRow* Ability = CharacterAbilities.Find(AbilityId);
		if (Ability && Ability->bEnabled && Ability->CharacterId == CanonicalId && Ability->Trigger == Trigger)
		{
			Result.Add(*Ability);
		}
	}
	return Result;
}

const FReEchoCsvCardRow* FReEchoCsvDataSnapshot::FindCard(const FName CardId) const
{
	return Cards.Find(CardId);
}

TArray<FReEchoCsvCardRow> FReEchoCsvDataSnapshot::GetOfferableCards(const FName OfferGroup) const
{
	TArray<FReEchoCsvCardRow> Result;
	for (const FName CardId : CardOrder)
	{
		const FReEchoCsvCardRow* Card = Cards.Find(CardId);
		if (Card && Card->bEnabled && Card->bOfferable && Card->OfferGroup == OfferGroup)
		{
			Result.Add(*Card);
		}
	}
	return Result;
}

const FReEchoCsvElementRow* FReEchoCsvDataSnapshot::FindElement(const FName ElementId) const
{
	return Elements.Find(ElementId);
}

const FReEchoCsvElementRow* FReEchoCsvDataSnapshot::FindElement(const EReEchoElement Element) const
{
	for (const FName ElementId : ElementOrder)
	{
		const FReEchoCsvElementRow* Row = Elements.Find(ElementId);
		if (Row && Row->Element == Element)
		{
			return Row;
		}
	}
	return nullptr;
}

const FReEchoCsvStatusRow* FReEchoCsvDataSnapshot::FindStatus(const FName StatusId) const
{
	return Statuses.Find(StatusId);
}

const FReEchoCsvReactionRow* FReEchoCsvDataSnapshot::FindReaction(const FName TriggerElementId,
                                                                  const FName AttachmentElementId) const
{
	for (const FName ReactionId : ReactionOrder)
	{
		const FReEchoCsvReactionRow* Reaction = Reactions.Find(ReactionId);
		if (Reaction && Reaction->bEnabled && Reaction->TriggerElementId == TriggerElementId &&
		    Reaction->AttachmentElementId == AttachmentElementId)
		{
			return Reaction;
		}
	}
	return nullptr;
}

const FReEchoCsvWeaponTypeRow* FReEchoCsvDataSnapshot::FindWeaponType(const FName WeaponTypeId) const
{
	return WeaponTypes.Find(WeaponTypeId);
}

const FReEchoCsvWeaponRow* FReEchoCsvDataSnapshot::FindWeapon(const FName WeaponId) const
{
	return Weapons.Find(WeaponId);
}

const FReEchoCsvWeaponRow* FReEchoCsvDataSnapshot::FindEnabledWeapon(const FName WeaponId) const
{
	const FReEchoCsvWeaponRow* Weapon = FindWeapon(WeaponId);
	return Weapon && Weapon->bEnabled ? Weapon : nullptr;
}

const FReEchoCsvWeaponRow* FReEchoCsvDataSnapshot::FindWeaponByInputSlot(const EReEchoInputSlot InputSlot) const
{
	for (const FName WeaponId : WeaponOrder)
	{
		const FReEchoCsvWeaponRow* Weapon = Weapons.Find(WeaponId);
		if (Weapon && Weapon->bEnabled && Weapon->InputSlot == InputSlot)
		{
			return Weapon;
		}
	}
	return nullptr;
}

TArray<FReEchoCsvWeaponRow> FReEchoCsvDataSnapshot::GetStartSelectableWeapons() const
{
	TArray<FReEchoCsvWeaponRow> Result;
	for (const FName WeaponId : WeaponOrder)
	{
		const FReEchoCsvWeaponRow* Weapon = Weapons.Find(WeaponId);
		if (Weapon && Weapon->bEnabled && Weapon->bStartSelectable)
		{
			Result.Add(*Weapon);
		}
	}
	Result.Sort(
	    [](const FReEchoCsvWeaponRow& Left, const FReEchoCsvWeaponRow& Right)
	    {
		    return Left.LoadoutOrder == Right.LoadoutOrder ? Left.Id.LexicalLess(Right.Id)
		                                                   : Left.LoadoutOrder < Right.LoadoutOrder;
	    });
	return Result;
}

TArray<FReEchoCsvAttackStepRow> FReEchoCsvDataSnapshot::GetAttackSteps(const FName AttackPatternId) const
{
	TArray<FReEchoCsvAttackStepRow> Result;
	for (const FName StepId : AttackStepOrder)
	{
		const FReEchoCsvAttackStepRow* Step = AttackSteps.Find(StepId);
		if (Step && Step->bEnabled && Step->AttackPatternId == AttackPatternId)
		{
			Result.Add(*Step);
		}
	}
	Result.Sort(
	    [](const FReEchoCsvAttackStepRow& Left, const FReEchoCsvAttackStepRow& Right)
	    {
		    return Left.StepIndex == Right.StepIndex ? Left.Id.LexicalLess(Right.Id) : Left.StepIndex < Right.StepIndex;
	    });
	return Result;
}

const FReEchoCsvEnemyRow* FReEchoCsvDataSnapshot::FindEnemy(const FName EnemyId) const
{
	return Enemies.Find(EnemyId);
}

const FReEchoCsvEnemyRow* FReEchoCsvDataSnapshot::FindEnabledEnemy(const FName EnemyId) const
{
	const FReEchoCsvEnemyRow* Enemy = FindEnemy(EnemyId);
	return Enemy && Enemy->bEnabled ? Enemy : nullptr;
}

const FReEchoCsvEnemyShardDropRow* FReEchoCsvDataSnapshot::FindEnemyShardDrop(const int32 InEncounterIndex) const
{
	return EnemyShardDrops.Find(InEncounterIndex);
}

const FReEchoCsvEnemyCombatStatRow* FReEchoCsvDataSnapshot::FindEnemyCombatStat(const FName EnemyId,
                                                                                const int32 CombatIndex) const
{
	const FName CompositeKey = FName(*FString::Printf(TEXT("%s#%d"), *EnemyId.ToString(), CombatIndex));
	return EnemyCombatStats.Find(CompositeKey);
}

const FReEchoCsvStageRow* FReEchoCsvDataSnapshot::FindStage(const FName StageId) const
{
	return Stages.Find(StageId);
}

const FReEchoCsvEncounterRow* FReEchoCsvDataSnapshot::FindEncounter(const FName EncounterId) const
{
	return Encounters.Find(EncounterId);
}

const FReEchoCsvEncounterRow* FReEchoCsvDataSnapshot::FindEncounterByIndex(const int32 EncounterIndex) const
{
	for (const FName EncounterId : EncounterOrder)
	{
		const FReEchoCsvEncounterRow* Encounter = Encounters.Find(EncounterId);
		if (Encounter && Encounter->bEnabled && Encounter->EncounterIndex == EncounterIndex)
		{
			return Encounter;
		}
	}
	return nullptr;
}

TArray<FReEchoCsvEncounterWaveRow> FReEchoCsvDataSnapshot::GetEncounterWaves(const FName EncounterId) const
{
	TArray<FReEchoCsvEncounterWaveRow> Result;
	for (const FName WaveId : EncounterWaveOrder)
	{
		const FReEchoCsvEncounterWaveRow* Wave = EncounterWaves.Find(WaveId);
		if (Wave && Wave->bEnabled && Wave->EncounterId == EncounterId)
		{
			Result.Add(*Wave);
		}
	}
	Result.Sort(
	    [](const FReEchoCsvEncounterWaveRow& Left, const FReEchoCsvEncounterWaveRow& Right)
	    {
		    return Left.WaveIndex < Right.WaveIndex;
	    });
	return Result;
}

const FReEchoCsvSpawnProfileRow* FReEchoCsvDataSnapshot::FindSpawnProfileByRole(const FName EnemyRole) const
{
	for (const FName ProfileId : SpawnProfileOrder)
	{
		const FReEchoCsvSpawnProfileRow* Profile = SpawnProfiles.Find(ProfileId);
		if (Profile && Profile->bEnabled && Profile->EnemyRole == EnemyRole)
		{
			return Profile;
		}
	}
	return nullptr;
}

const FReEchoCsvSpawnPolicyRow* FReEchoCsvDataSnapshot::FindEnabledSpawnPolicy() const
{
	for (const TPair<FName, FReEchoCsvSpawnPolicyRow>& Pair : SpawnPolicies)
	{
		if (Pair.Value.bEnabled)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

const FReEchoCsvAttributeRow* FReEchoCsvDataSnapshot::FindAttribute(FName AttributeId) const
{
	return Attributes.Find(AttributeId);
}

const TArray<FName>& FReEchoCsvDataSnapshot::GetAttributeOrder() const
{
	return AttributeOrder;
}

FString FReEchoCsvLoadResult::FormatIssues() const
{
	TArray<FString> Lines;
	for (const FReEchoCsvIssue& Issue : Issues)
	{
		Lines.Add(Issue.ToString());
	}
	return FString::Join(Lines, TEXT("\n"));
}

void FReEchoCsvDataRegistry::EnsureDefaultRegistrations()
{
	FScopeLock Lock(&RegistryCriticalSection);
	if (bDefaultRegistrationsReady)
	{
		return;
	}
	RegisteredBehaviorIds.Add(FName(BehaviorNone));
	RegisteredBehaviorIds.Add(FName(DefaultBehaviorId));
	RegisteredEffectKinds.Add(FName(DefaultEffectKind));
	RegisteredEffectKinds.Add(FName(StatModifierEffectKind));
	RegisteredEffectKinds.Add(FName(InstantRecoveryEffectKind));
	RegisteredEffectKinds.Add(FName(CardBehaviorEffectKind));
	RegisteredEffectKinds.Add(FName(ElementReactionEffectKind));
	RegisteredEffectKinds.Add(TEXT("ExtraCardChoice"));
	RegisteredFormulaIds.Add(FName(BehaviorNone));
	RegisteredAttackPatternIds.Add(FName(BehaviorNone));
	bDefaultRegistrationsReady = true;
}

void FReEchoCsvDataRegistry::RegisterBuiltInCsvBehaviors()
{
	RegisterBehaviorId(TEXT("Character.StaticStat"));
	RegisterBehaviorId(TEXT("Character.PersistentGrowth"));
	RegisterBehaviorId(TEXT("Character.EveryNth"));
	RegisterBehaviorId(TEXT("Character.MissingHealthSteps"));
	RegisterBehaviorId(TEXT("Card.StatModifier"));
	RegisterBehaviorId(TEXT("Card.InstantRecovery"));
	for (const FName BehaviorId : {
	         FName(TEXT("Card.GrantTier")),
	         FName(TEXT("Card.RandomStatTrade")),
	         FName(TEXT("Card.RandomRateTrade")),
	         FName(TEXT("Card.ResetRunes")),
	         FName(TEXT("Card.FreeShopVisit")),
	         FName(TEXT("Card.UnlimitedShopRefresh")),
	         FName(TEXT("Card.CollectCores")),
	         FName(TEXT("Card.TrackNextKills")),
	         FName(TEXT("Card.TrackNextReactions")),
	         FName(TEXT("Card.EchoElementAura")),
	         FName(TEXT("Card.EchoSlowAura")),
	         FName(TEXT("Card.ElementAttachedCrit")),
	         FName(TEXT("Card.ReactionHeal")),
	         FName(TEXT("Card.DamageSubstitution")),
	         FName(TEXT("Card.NextShardDrop")),
	         FName(TEXT("Card.ShopContract")),
	         FName(TEXT("Card.EncounterStun")),
	         FName(TEXT("Card.DoubleEcho")),
	         FName(TEXT("Card.TimeAnchor")),
	         FName(TEXT("Card.SoloBody")),
	         FName(TEXT("Card.TauntEcho")),
	         FName(TEXT("Card.SoulResonance")),
	         FName(TEXT("Card.EnemyImmunity")),
	         FName(TEXT("Card.CriticalElement")),
	         FName(TEXT("Card.ElementCanCrit")),
	         FName(TEXT("Card.DistanceDamage")),
	         FName(TEXT("Card.DoubleCritRoll")),
	         FName(TEXT("Card.BloodForging")),
	         FName(TEXT("Card.EndKillRefresh")),
	         FName(TEXT("Card.HarvestPenalty")),
	         FName(TEXT("Card.ShardOutgoingDamage")),
	         FName(TEXT("Card.ShardIncomingBarrier")),
	         FName(TEXT("Card.ReactionDiversity")),
	         FName(TEXT("Card.DoubleNonCoreSlots")),
	         FName(TEXT("Card.NumericChallenge")),
	         FName(TEXT("Card.OverkillHeal")),
	         FName(TEXT("Card.SelfRace")),
	         FName(TEXT("Card.NegativeStatusHeal")),
	         FName(TEXT("Card.TargetKillCurse")),
	         FName(TEXT("Card.RecordReaction")),
	         FName(TEXT("Card.CurseBank")),
	         FName(TEXT("Card.ConnectionLine")),
	         FName(TEXT("Card.ProximityDamage")),
	         FName(TEXT("Card.WeaponMaster")),
	         FName(TEXT("Card.EchoTrinityHead")),
	         FName(TEXT("Card.EchoTrinityBody")),
	         FName(TEXT("Card.EchoTrinityLegs")),
	         FName(TEXT("Card.InfiniteStackingBurn")),
	         FName(TEXT("Card.VaporizeWaterSplash")),
	         FName(TEXT("Card.ConductDamageGrowth")),
	         FName(TEXT("Card.OverhealCapacity")),
	         FName(TEXT("Card.AlternatingSources")),
	         FName(TEXT("Card.EasterShardSwing")),
	         FName(TEXT("Card.EasterShardThreshold")),
	         FName(TEXT("Card.EasterIndependentGrant")),
	         FName(TEXT("Card.EasterEchoContact")),
	         FName(TEXT("Card.EasterShardSacrifice")),
	         FName(TEXT("Card.EasterRandomStun")),
	         FName(TEXT("Card.EasterDamageCards")),
	         FName(TEXT("Card.EasterPhysicalLottery")),
	         FName(TEXT("Card.EasterShardComparison")),
	         FName(TEXT("Card.EasterAttendance")),
	         FName(TEXT("Card.ExpectedOutcome")),
	         FName(TEXT("Card.KillThresholdStatBoost")),
	         FName(TEXT("Card.KillThresholdImmunity")),
	         FName(TEXT("Card.EndKillShards")),
	         FName(TEXT("Card.CollectCoresGrantTiered")),
	         FName(TEXT("Card.GrantAllTier1")),
	     })
	{
		RegisterBehaviorId(BehaviorId);
	}
	RegisterBehaviorId(TEXT("Status.ElementImmunity"));
	RegisterBehaviorId(TEXT("Status.Burn"));
	RegisterBehaviorId(TEXT("Status.Stun"));
	RegisterBehaviorId(TEXT("Status.Bleeding"));
	RegisterBehaviorId(TEXT("Status.Cursed"));
	RegisterBehaviorId(TEXT("Reaction.Burn"));
	RegisterBehaviorId(TEXT("Reaction.Vaporize"));
	RegisterBehaviorId(TEXT("Reaction.Growth"));
	RegisterBehaviorId(TEXT("Reaction.Conduct"));
	RegisterBehaviorId(TEXT("Reaction.Enhance"));
	RegisterFormulaId(TEXT("Element.ElementAttackDot"));
	RegisterFormulaId(TEXT("Element.ElementAttackSquared"));
	RegisterFormulaId(TEXT("Element.AttachInRadius"));
	RegisterFormulaId(TEXT("Element.ChainElementAttack"));
	RegisterFormulaId(TEXT("Element.EnhanceNextReaction"));
	RegisterBehaviorId(TEXT("Weapon.AttackStep"));
	RegisterBehaviorId(TEXT("Weapon.DashStrike"));
	RegisterBehaviorId(TEXT("Part.CoreDamageChannel"));
	RegisterBehaviorId(TEXT("Part.StatModifier"));
	RegisterBehaviorId(TEXT("Part.AttackPatternReplacement"));
	RegisterBehaviorId(TEXT("Part.OnKillHealPercent"));
	for (const FName BehaviorId : {
	         FName(TEXT("Part.ProjectileSplitOnHit")),
	         FName(TEXT("Part.ProjectilePierceOnCritical")),
	         FName(TEXT("Part.ApplyBleedOnCritical")),
	         FName(TEXT("Part.DropShardOnKill")),
	         FName(TEXT("Part.MoveSpeedOnKill")),
	         FName(TEXT("Part.AttackMoveSpeedOnAttack")),
	         FName(TEXT("Part.RangeOnGroupHit")),
	         FName(TEXT("Part.HealOnHit")),
	         FName(TEXT("Part.StunOnHit")),
	         FName(TEXT("Part.BleedEveryTargetHits")),
	         FName(TEXT("Part.OuterRingDamage")),
	         FName(TEXT("Part.MoveSpeedPerHit")),
	         FName(TEXT("Part.InvulnerableOnGroupHit")),
	         FName(TEXT("Part.AttackSpeedPerHit")),
	         FName(TEXT("Part.ScytheThrowRecall")),
	         FName(TEXT("Part.DropShardEveryHits")),
	         FName(TEXT("Part.MeteorOnGroupHit")),
	         FName(TEXT("Part.ApplyBleedOnHitChance")),
	         FName(TEXT("Part.AttackSpeedOnAttack")),
	         FName(TEXT("Part.MoveSpeedOnAttack")),
	     })
	{
		RegisterBehaviorId(BehaviorId);
	}
	RegisterBehaviorId(TEXT("Enemy.Grunt"));
	RegisterBehaviorId(TEXT("Enemy.Shield"));
	RegisterBehaviorId(TEXT("Enemy.Bomber"));
	RegisterBehaviorId(TEXT("Enemy.Slime"));
	RegisterBehaviorId(TEXT("Enemy.Ranged"));
	RegisterBehaviorId(TEXT("Enemy.Elite"));
	RegisterBehaviorId(TEXT("Boss.TimeGuard"));
	RegisterBehaviorId(TEXT("Boss.MeleeSweep"));
	RegisterBehaviorId(TEXT("Boss.Projectile"));
	RegisterBehaviorId(TEXT("Boss.BlinkSlam"));
	RegisterBehaviorId(TEXT("Boss.PrayerBeam"));
	RegisterBehaviorId(TEXT("Boss.ElementCleanse"));
	RegisterBehaviorId(TEXT("Enemy.RangedBurst"));
	RegisterBehaviorId(TEXT("Enemy.EliteDash"));
	RegisterEffectKind(TEXT("WeaponDamageChannel"));
	RegisterEffectKind(TEXT("AttackPatternReplacement"));
	RegisterEffectKind(TEXT("ParameterizedBehavior"));
	RegisterEffectKind(TEXT("StatModifier"));
	RegisterEffectKind(TEXT("UniqueBehavior"));
	RegisterFormulaId(TEXT("Weapon.DamageCoefficient"));
	RegisterAttackPatternId(TEXT("Pattern.LongSwordCombo"));
	RegisterAttackPatternId(TEXT("Pattern.ScytheSweep"));
	RegisterAttackPatternId(TEXT("Pattern.BowShot"));
	RegisterAttackPatternId(TEXT("Pattern.GunShot"));
	RegisterAttackPatternId(TEXT("Pattern.MoonStaffWave"));
	RegisterAttackPatternId(TEXT("Pattern.ElementalProjectile"));
}

void FReEchoCsvDataRegistry::RegisterBehaviorId(const FName BehaviorId)
{
	EnsureDefaultRegistrations();
	if (!BehaviorId.IsNone())
	{
		FScopeLock Lock(&RegistryCriticalSection);
		RegisteredBehaviorIds.Add(BehaviorId);
	}
}

void FReEchoCsvDataRegistry::RegisterEffectKind(const FName EffectKind)
{
	EnsureDefaultRegistrations();
	if (!EffectKind.IsNone())
	{
		FScopeLock Lock(&RegistryCriticalSection);
		RegisteredEffectKinds.Add(EffectKind);
	}
}

void FReEchoCsvDataRegistry::RegisterFormulaId(const FName FormulaId)
{
	EnsureDefaultRegistrations();
	if (!FormulaId.IsNone())
	{
		FScopeLock Lock(&RegistryCriticalSection);
		RegisteredFormulaIds.Add(FormulaId);
	}
}

void FReEchoCsvDataRegistry::RegisterAttackPatternId(const FName AttackPatternId)
{
	EnsureDefaultRegistrations();
	if (!AttackPatternId.IsNone())
	{
		FScopeLock Lock(&RegistryCriticalSection);
		RegisteredAttackPatternIds.Add(AttackPatternId);
	}
}

bool FReEchoCsvDataRegistry::IsBehaviorIdRegistered(const FName BehaviorId)
{
	EnsureDefaultRegistrations();
	FScopeLock Lock(&RegistryCriticalSection);
	return RegisteredBehaviorIds.Contains(BehaviorId);
}

bool FReEchoCsvDataRegistry::IsEffectKindRegistered(const FName EffectKind)
{
	EnsureDefaultRegistrations();
	FScopeLock Lock(&RegistryCriticalSection);
	return RegisteredEffectKinds.Contains(EffectKind);
}

bool FReEchoCsvDataRegistry::IsFormulaIdRegistered(const FName FormulaId)
{
	EnsureDefaultRegistrations();
	FScopeLock Lock(&RegistryCriticalSection);
	return RegisteredFormulaIds.Contains(FormulaId);
}

bool FReEchoCsvDataRegistry::IsAttackPatternIdRegistered(const FName AttackPatternId)
{
	EnsureDefaultRegistrations();
	FScopeLock Lock(&RegistryCriticalSection);
	return RegisteredAttackPatternIds.Contains(AttackPatternId);
}

FString FReEchoCsvDataRegistry::GetDefaultDataDirectory()
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
}

FReEchoCsvLoadResult FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(const FString& DataDirectory)
{
	EnsureDefaultRegistrations();

	FReEchoCsvLoadResult Result;
	TSharedRef<FReEchoCsvDataSnapshot> MutableSnapshot = MakeShared<FReEchoCsvDataSnapshot>();
	MutableSnapshot->SchemaVersion = SupportedSchemaVersion;

	TMap<FString, ReEchoCsv::FManifestEntry> ManifestEntries;
	ReEchoCsv::ReadManifest(DataDirectory, GetRequiredTableIds(), ManifestEntries, Result.Issues);
	if (Result.Issues.Num() == 0)
	{
		ReadRuntimeSmokeTable(DataDirectory, ManifestEntries[RuntimeSmokeTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReadRuntimeSmokeEffectsTable(
		    DataDirectory, ManifestEntries[RuntimeSmokeEffectsTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReEchoCharacterBuildCsv::ReadTables(DataDirectory, ManifestEntries, *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		CompileCardCatalog(DataDirectory, ManifestEntries, *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReEchoElementReactionCsv::ReadTables(DataDirectory, ManifestEntries, *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReEchoWeaponCsv::ReadTables(DataDirectory, ManifestEntries, *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReEchoEnemyCsv::ReadTables(DataDirectory, ManifestEntries, *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReEchoEncounterCsv::ReadTables(DataDirectory, ManifestEntries, *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReadAttributesTable(DataDirectory, ManifestEntries[AttributesTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0 && MutableSnapshot->RuntimeSmokeRows.Num() == 0)
	{
		ReEchoCsv::AddIssue(Result.Issues,
		                    FPaths::Combine(DataDirectory, ManifestEntries[RuntimeSmokeTableId].FileName),
		                    1,
		                    TEXT("Id"),
		                    TEXT("RuntimeSmoke must contain at least one row"));
	}

	if (Result.Issues.Num() == 0)
	{
		Result.bSuccess = true;
		Result.Snapshot = MutableSnapshot;
	}
	return Result;
}

FReEchoCsvLoadResult FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(const FString& DataDirectory)
{
	FReEchoCsvLoadResult Result = LoadSnapshotFromDirectory(DataDirectory);
	if (Result.bSuccess)
	{
		{
			FScopeLock Lock(&RegistryCriticalSection);
			PublishedSnapshot = Result.Snapshot;
		}
		ReEchoElementRuntime::PublishRuleSet(CompileElementRuleSet(*Result.Snapshot));
	}
	return Result;
}

FReEchoCsvLoadResult FReEchoCsvDataRegistry::LoadAndPublishDefault()
{
	return LoadAndPublishFromDirectory(GetDefaultDataDirectory());
}

TSharedPtr<const FReEchoCsvDataSnapshot> FReEchoCsvDataRegistry::GetSnapshot()
{
	FScopeLock Lock(&RegistryCriticalSection);
	return PublishedSnapshot;
}

void FReEchoCsvDataRegistry::ClearPublishedSnapshotForTests()
{
	{
		FScopeLock Lock(&RegistryCriticalSection);
		PublishedSnapshot.Reset();
	}
	ReEchoElementRuntime::ClearRuleSetForTests();
}
