#include "ReEchoCharacterBuildCsvReader.h"

#include "Cards/ReEchoCardCatalog.h"
#include "Misc/Paths.h"

namespace ReEchoCharacterBuildCsv
{
namespace
{
constexpr const TCHAR* CharactersTableId = TEXT("Characters");
constexpr const TCHAR* CharacterAliasesTableId = TEXT("CharacterAliases");
constexpr const TCHAR* CharacterAbilitiesTableId = TEXT("CharacterAbilities");
constexpr const TCHAR* CardsTableId = TEXT("Cards");
constexpr const TCHAR* CardEffectsTableId = TEXT("CardEffects");
constexpr const TCHAR* BehaviorNone = TEXT("None");

TArray<FName> ParseNameList(const FString& Text)
{
	TArray<FName> Result;
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT("|"), true);
	for (const FString& Part : Parts)
	{
		const FString Trimmed = Part.TrimStartAndEnd();
		if (!Trimmed.IsEmpty())
		{
			Result.Add(FName(*Trimmed));
		}
	}
	return Result;
}

bool IsRegisteredOrNone(const FName BehaviorId)
{
	return BehaviorId == BehaviorNone || FReEchoCsvDataRegistry::IsBehaviorIdRegistered(BehaviorId);
}

bool IsAllowedCardEffectTarget(const FName Target)
{
	static const TSet<FName> AllowedTargets = {
	    TEXT("HpMax"),
	    TEXT("HpPoint"),
	    TEXT("PhysicalAttack"),
	    TEXT("ElementalAttack"),
	    TEXT("Block"),
	    TEXT("AttackSpeed"),
	    TEXT("MovementSpeed"),
	    TEXT("EchoEfficiency"),
	    TEXT("CriticalRate"),
	    TEXT("CriticalEffect"),
	    TEXT("ReactionEfficiency"),
	    TEXT("Tier"),
	    TEXT("PhysicalOrElemental"),
	    TEXT("ReactionOrCritical"),
	    TEXT("Water"),
	    TEXT("Grass"),
	    TEXT("Damage"),
	    TEXT("TimeShards"),
	    TEXT("ShopDiscount"),
	    TEXT("HpMaxAndPoint"),
	    TEXT("Stun"),
	    TEXT("EchoCount"),
	    TEXT("AnchorRecording"),
	    TEXT("AllBaseStats"),
	    TEXT("EchoHealth"),
	    TEXT("PhysicalAndElementalAttack"),
	    TEXT("EnemyElementImmunity"),
	    TEXT("RandomElement"),
	    TEXT("ElementCanCrit"),
	    TEXT("CriticalRollCount"),
	    TEXT("MinimumGuaranteedTier"),
	    TEXT("FreeShopRefresh"),
	    TEXT("NonCoreSlotCapacity"),
	    TEXT("ShopPrice"),
	    TEXT("WeaponRunes"),
	    TEXT("WeaponRuneShop"),
	    TEXT("CoreCollection"),
	    TEXT("Status"),
	    TEXT("ShopCredit"),
	    TEXT("ConnectionLine"),
	    TEXT("WeaponHistory"),
	};
	return AllowedTargets.Contains(Target);
}

bool IsAllowedEffectBehaviorPair(const FName EffectKind, const FName BehaviorId)
{
	if (EffectKind == TEXT("StatModifier"))
	{
		return BehaviorId == TEXT("Card.StatModifier");
	}
	if (EffectKind == TEXT("InstantRecovery"))
	{
		return BehaviorId == TEXT("Card.InstantRecovery");
	}
	if (EffectKind == TEXT("CardBehavior"))
	{
		return FReEchoCardCatalog::IsSupportedBehavior(BehaviorId) && BehaviorId != TEXT("Card.StatModifier") &&
		       BehaviorId != TEXT("Card.InstantRecovery");
	}
	return false;
}

bool ReadCharactersTable(const FString& DataDirectory,
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
	                            TEXT("SourceWorkbookId"),
	                            TEXT("DisplayName"),
	                            TEXT("Description"),
	                            TEXT("Enabled"),
	                            TEXT("DisabledReason"),
	                            TEXT("RoleId"),
	                            TEXT("PromotionPriority"),
	                            TEXT("DefaultWeaponId"),
	                            TEXT("AppearanceId"),
	                            TEXT("HpMax"),
	                            TEXT("PhysicalAttack"),
	                            TEXT("ElementalAttack"),
	                            TEXT("AttackSpeed"),
	                            TEXT("MovementSpeed"),
	                            TEXT("CriticalRate"),
	                            TEXT("CriticalEffect"),
	                            TEXT("EchoEfficiency"),
	                            TEXT("ReactionEfficiency")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvCharacterRow Character;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Character.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("SourceWorkbookId"), Character.SourceWorkbookId, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Character.DisplayName, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Description"), Character.Description, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Character.bEnabled, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Character.DisabledReason);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("RoleId"), Character.RoleId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("PromotionPriority"), Character.PromotionPriority, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("DefaultWeaponId"), Character.DefaultWeaponId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("AppearanceId"), Character.AppearanceId, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("HpMax"), 1.0f, 100000.0f, Character.BaseStats.HpMax, Issues);
		Character.BaseStats.HpPoint = Character.BaseStats.HpMax;
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("PhysicalAttack"), 0.0f, 100000.0f, Character.BaseStats.PhysicalAttack, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ElementalAttack"), 0.0f, 100000.0f, Character.BaseStats.ElementalAttack, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("AttackSpeed"), 0.1f, 100.0f, Character.BaseStats.AttackSpeed, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("MovementSpeed"), 0.1f, 100.0f, Character.BaseStats.MovementSpeed, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("CriticalRate"), 0.0f, 1.0f, Character.BaseStats.CriticalRate, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("CriticalEffect"), 0.0f, 100.0f, Character.BaseStats.CriticalEffect, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("EchoEfficiency"), 0.0f, 100.0f, Character.BaseStats.EchoEfficiency, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("ReactionEfficiency"), 0.0f, 100.0f, Character.BaseStats.ReactionEfficiency, Issues);

		if (Character.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Character.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (Character.bEnabled && Character.RoleId == TEXT("Disabled"))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("RoleId"), TEXT("Enabled character cannot use Disabled role"));
		}
		if (!Character.bEnabled && Character.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(Character.Id);
		Snapshot.Characters.Add(Character.Id, Character);
	}
	return Issues.Num() == 0;
}

bool IsValidCharacterAbilityDefinition(const FReEchoCsvCharacterAbilityRow& Ability)
{
	if (Ability.ValueOp != EReEchoCsvValueOp::Add)
	{
		return false;
	}
	if (Ability.BehaviorId == TEXT("Character.StaticStat"))
	{
		return Ability.Trigger == TEXT("OnBuildInitialized") && Ability.EffectKind == TEXT("StatModifier") &&
		       (Ability.Target == TEXT("MovementSpeed") || Ability.Target == TEXT("CriticalRate") ||
		        Ability.Target == TEXT("CriticalEffect")) &&
		       FMath::IsNearlyZero(Ability.Interval);
	}
	if (Ability.BehaviorId == TEXT("Character.PersistentGrowth"))
	{
		return Ability.Trigger == TEXT("OnEncounterCompleted") && Ability.EffectKind == TEXT("StatModifier") &&
		       Ability.Target == TEXT("ReactionEfficiency") && FMath::IsNearlyZero(Ability.Interval);
	}
	if (Ability.BehaviorId == TEXT("Character.EveryNth"))
	{
		return Ability.Trigger == TEXT("OnTraitChoiceApplied") && Ability.EffectKind == TEXT("ExtraCardChoice") &&
		       Ability.Target == TEXT("TraitCardChoice") && Ability.Interval >= 1.0f &&
		       FMath::IsNearlyEqual(Ability.Interval, FMath::RoundToFloat(Ability.Interval)) && Ability.Value > 0.0f &&
		       FMath::IsNearlyEqual(Ability.Value, FMath::RoundToFloat(Ability.Value));
	}
	if (Ability.BehaviorId == TEXT("Character.MissingHealthSteps"))
	{
		return Ability.Trigger == TEXT("OnHealthChanged") && Ability.EffectKind == TEXT("StatModifier") &&
		       (Ability.Target == TEXT("PhysicalAttack") || Ability.Target == TEXT("ElementalAttack")) &&
		       Ability.Value >= 0.0f && Ability.Interval > 0.0f && Ability.Interval <= 1.0f;
	}
	return false;
}

bool ReadCharacterAbilitiesTable(const FString& DataDirectory,
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
	                            TEXT("CharacterId"),
	                            TEXT("Order"),
	                            TEXT("Trigger"),
	                            TEXT("EffectKind"),
	                            TEXT("Target"),
	                            TEXT("ValueOp"),
	                            TEXT("Value"),
	                            TEXT("BehaviorId"),
	                            TEXT("Interval"),
	                            TEXT("Enabled"),
	                            TEXT("DisabledReason")},
	                           Issues);
	TSet<FName> SeenIds;
	TSet<FString> SeenOrderKeys;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvCharacterAbilityRow Ability;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Ability.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("CharacterId"), Ability.CharacterId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("Order"), Ability.Order, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Trigger"), Ability.Trigger, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("EffectKind"), Ability.EffectKind, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Target"), Ability.Target, Issues);
		ReEchoCsv::RequireValueOp(Table, Row, TEXT("ValueOp"), Ability.ValueOp, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("Value"), -100000.0f, 100000.0f, Ability.Value, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BehaviorId"), Ability.BehaviorId, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("Interval"), 0.0f, 100000.0f, Ability.Interval, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Ability.bEnabled, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Ability.DisabledReason);
		const FString OrderKey = FString::Printf(
		    TEXT("%s|%s|%d"), *Ability.CharacterId.ToString(), *Ability.Trigger.ToString(), Ability.Order);
		if (SeenIds.Contains(Ability.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (SeenOrderKeys.Contains(OrderKey))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("Order"), TEXT("Duplicate CharacterId/Trigger/Order"));
		}
		if (!Snapshot.Characters.Contains(Ability.CharacterId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("CharacterId"), TEXT("Unknown Characters.Id reference"));
		}
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(Ability.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!FReEchoCsvDataRegistry::IsEffectKindRegistered(Ability.EffectKind))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("EffectKind"), TEXT("Unknown registered C++ effect kind"));
		}
		if (!IsValidCharacterAbilityDefinition(Ability))
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    Row.Line,
			                    TEXT("BehaviorId"),
			                    TEXT("Unsupported character ability trigger/effect/target/value combination"));
		}
		if (!Ability.bEnabled && Ability.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(Ability.Id);
		SeenOrderKeys.Add(OrderKey);
		Snapshot.CharacterAbilities.Add(Ability.Id, Ability);
		Snapshot.CharacterAbilityOrder.Add(Ability.Id);
	}
	Snapshot.CharacterAbilityOrder.Sort(
	    [&Snapshot](const FName LeftId, const FName RightId)
	    {
		    const FReEchoCsvCharacterAbilityRow& Left = Snapshot.CharacterAbilities.FindChecked(LeftId);
		    const FReEchoCsvCharacterAbilityRow& Right = Snapshot.CharacterAbilities.FindChecked(RightId);
		    if (Left.CharacterId != Right.CharacterId)
		    {
			    return Left.CharacterId.LexicalLess(Right.CharacterId);
		    }
		    if (Left.Trigger != Right.Trigger)
		    {
			    return Left.Trigger.LexicalLess(Right.Trigger);
		    }
		    return Left.Order == Right.Order ? Left.Id.LexicalLess(Right.Id) : Left.Order < Right.Order;
	    });
	return Issues.Num() == 0;
}

bool ReadCharacterAliasesTable(const FString& DataDirectory,
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

	ReEchoCsv::HasExactColumns(Table, {TEXT("Id"), TEXT("CanonicalCharacterId"), TEXT("Reason")}, Issues);
	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FName AliasId;
		FName CanonicalId;
		FString Reason;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), AliasId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("CanonicalCharacterId"), CanonicalId, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Reason"), Reason, Issues);
		if (SeenIds.Contains(AliasId))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!Snapshot.Characters.Contains(CanonicalId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("CanonicalCharacterId"), TEXT("Unknown Characters.Id reference"));
		}
		SeenIds.Add(AliasId);
		Snapshot.CharacterAliases.Add(AliasId, CanonicalId);
	}
	return Issues.Num() == 0;
}

bool ReadCardsTable(const FString& DataDirectory,
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
	                            TEXT("SourceWorkbookId"),
	                            TEXT("Tier"),
	                            TEXT("DisplayName"),
	                            TEXT("Description"),
	                            TEXT("Tags"),
	                            TEXT("PromotionRoleId"),
	                            TEXT("OfferGroup"),
	                            TEXT("Enabled"),
	                            TEXT("Offerable"),
	                            TEXT("StackPolicy"),
	                            TEXT("ConflictPolicy"),
	                            TEXT("ReviewStatus"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvCardRow Card;
		FString Tags;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Card.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("SourceWorkbookId"), Card.SourceWorkbookId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("Tier"), Card.Tier, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Card.DisplayName, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Description"), Card.Description, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Tags"), Tags, Issues);
		Card.Tags = ParseNameList(Tags);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("PromotionRoleId"), Card.PromotionRoleId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("OfferGroup"), Card.OfferGroup, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Card.bEnabled, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Offerable"), Card.bOfferable, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("StackPolicy"), Card.StackPolicy, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ConflictPolicy"), Card.ConflictPolicy, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ReviewStatus"), Card.ReviewStatus, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Card.DisabledReason);

		if (Card.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Card.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (Card.bEnabled && Card.ReviewStatus != TEXT("Approved"))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("ReviewStatus"), TEXT("Enabled card must be Approved"));
		}
		if (!Card.bEnabled && Card.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		if (Card.bOfferable && !Card.bEnabled)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("Offerable"), TEXT("Offerable card must be enabled"));
		}
		SeenIds.Add(Card.Id);
		Snapshot.CardOrder.Add(Card.Id);
		Snapshot.Cards.Add(Card.Id, Card);
	}
	return Issues.Num() == 0;
}

bool ReadCardEffectsTable(const FString& DataDirectory,
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
	                            TEXT("CardId"),
	                            TEXT("Order"),
	                            TEXT("Trigger"),
	                            TEXT("EffectKind"),
	                            TEXT("Target"),
	                            TEXT("ValueOp"),
	                            TEXT("Value"),
	                            TEXT("BehaviorId"),
	                            TEXT("ParamName"),
	                            TEXT("ParamValue")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FString> SeenCardOrders;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvCardEffectRow Effect;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Effect.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("CardId"), Effect.CardId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("Order"), Effect.Order, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Trigger"), Effect.Trigger, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("EffectKind"), Effect.EffectKind, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Target"), Effect.Target, Issues);
		ReEchoCsv::RequireValueOp(Table, Row, TEXT("ValueOp"), Effect.ValueOp, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("Value"), -100000.0f, 100000.0f, Effect.Value, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BehaviorId"), Effect.BehaviorId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("ParamName"), Effect.ParamName, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("ParamValue"), -100000.0f, 100000.0f, Effect.ParamValue, Issues);

		if (SeenIds.Contains(Effect.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}

		const FString CardOrderKey = FString::Printf(TEXT("%s:%d"), *Effect.CardId.ToString(), Effect.Order);
		if (SeenCardOrders.Contains(CardOrderKey))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Order"), TEXT("Duplicate CardId/Order"));
		}
		SeenCardOrders.Add(CardOrderKey);

		if (!FReEchoCardCatalog::IsSupportedTrigger(Effect.Trigger))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Trigger"), TEXT("Unsupported card effect trigger"));
		}
		if (!IsAllowedCardEffectTarget(Effect.Target))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Target"), TEXT("Unsupported card effect target"));
		}
		if (!FReEchoCsvDataRegistry::IsEffectKindRegistered(Effect.EffectKind))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("EffectKind"), TEXT("Unknown registered C++ effect kind"));
		}
		if (!IsRegisteredOrNone(Effect.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!IsAllowedEffectBehaviorPair(Effect.EffectKind, Effect.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("EffectKind/BehaviorId combination is invalid"));
		}

		if (FReEchoCsvCardRow* Card = Snapshot.Cards.Find(Effect.CardId))
		{
			Card->Effects.Add(Effect);
		}
		else
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("CardId"), TEXT("Unknown Cards.Id reference"));
		}
		SeenIds.Add(Effect.Id);
	}

	for (auto& CardPair : Snapshot.Cards)
	{
		CardPair.Value.Effects.Sort(
		    [](const FReEchoCsvCardEffectRow& Left, const FReEchoCsvCardEffectRow& Right)
		    {
			    return Left.Order < Right.Order;
		    });
		if (CardPair.Value.bEnabled && CardPair.Value.Effects.Num() == 0)
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    1,
			                    TEXT("CardId"),
			                    FString::Printf(TEXT("Enabled card %s has no effect rows"), *CardPair.Key.ToString()));
		}
	}
	return Issues.Num() == 0;
}
}

bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues)
{
	ReadCharactersTable(DataDirectory, ManifestEntries[CharactersTableId], Snapshot, Issues);
	if (Issues.Num() == 0)
	{
		ReadCharacterAliasesTable(DataDirectory, ManifestEntries[CharacterAliasesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadCharacterAbilitiesTable(DataDirectory, ManifestEntries[CharacterAbilitiesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadCardsTable(DataDirectory, ManifestEntries[CardsTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadCardEffectsTable(DataDirectory, ManifestEntries[CardEffectsTableId], Snapshot, Issues);
	}
	return Issues.Num() == 0;
}
}
