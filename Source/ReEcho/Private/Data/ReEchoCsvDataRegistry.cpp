#include "Data/ReEchoCsvDataRegistry.h"

#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "ReEcho.h"
#include "ReEchoCsvDataReader.h"

namespace
{
constexpr const TCHAR* RuntimeSmokeTableId = TEXT("RuntimeSmoke");
constexpr const TCHAR* RuntimeSmokeEffectsTableId = TEXT("RuntimeSmokeEffects");
constexpr const TCHAR* CharactersTableId = TEXT("Characters");
constexpr const TCHAR* CharacterAliasesTableId = TEXT("CharacterAliases");
constexpr const TCHAR* CardsTableId = TEXT("Cards");
constexpr const TCHAR* CardEffectsTableId = TEXT("CardEffects");

constexpr const TCHAR* BehaviorNone = TEXT("None");
constexpr const TCHAR* DefaultBehaviorId = TEXT("RuntimeSmoke.LogValue");
constexpr const TCHAR* DefaultEffectKind = TEXT("ScalarModifier");
constexpr const TCHAR* StatModifierEffectKind = TEXT("StatModifier");
constexpr const TCHAR* InstantRecoveryEffectKind = TEXT("InstantRecovery");

FCriticalSection RegistryCriticalSection;
TSharedPtr<const FReEchoCsvDataSnapshot> PublishedSnapshot;
TSet<FName> RegisteredBehaviorIds;
TSet<FName> RegisteredEffectKinds;
bool bDefaultRegistrationsReady = false;

TArray<FString> GetRequiredTableIds()
{
	return {RuntimeSmokeTableId,
	        RuntimeSmokeEffectsTableId,
	        CharactersTableId,
	        CharacterAliasesTableId,
	        CardsTableId,
	        CardEffectsTableId};
}

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
	                            TEXT("Enabled"),
	                            TEXT("DisabledReason"),
	                            TEXT("RoleId"),
	                            TEXT("PromotionPriority"),
	                            TEXT("DefaultWeaponId"),
	                            TEXT("AppearanceId"),
	                            TEXT("PassiveBehaviorId"),
	                            TEXT("PassiveValue"),
	                            TEXT("HpMax"),
	                            TEXT("PhysicalAttack"),
	                            TEXT("ElementalAttack"),
	                            TEXT("AttackSpeed"),
	                            TEXT("MovementSpeed"),
	                            TEXT("CriticalRate"),
	                            TEXT("CriticalEffect"),
	                            TEXT("EchoEfficiency"),
	                            TEXT("ReactionEfficiency"),
	                            TEXT("EverySecondAttackBonus"),
	                            TEXT("RandomElementProjectiles")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvCharacterRow Character;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Character.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("SourceWorkbookId"), Character.SourceWorkbookId, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Character.DisplayName, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Character.bEnabled, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Character.DisabledReason);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("RoleId"), Character.RoleId, Issues);
		ReEchoCsv::RequireInt(Table, Row, TEXT("PromotionPriority"), Character.PromotionPriority, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("DefaultWeaponId"), Character.DefaultWeaponId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("AppearanceId"), Character.AppearanceId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("PassiveBehaviorId"), Character.PassiveBehaviorId, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("PassiveValue"), 0.0f, 100000.0f, Character.PassiveValue, Issues);
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
		ReEchoCsv::RequireFloat(Table,
		                        Row,
		                        TEXT("EverySecondAttackBonus"),
		                        0.0f,
		                        100.0f,
		                        Character.BaseStats.EverySecondAttackBonus,
		                        Issues);
		ReEchoCsv::RequireBool(
		    Table, Row, TEXT("RandomElementProjectiles"), Character.BaseStats.bRandomElementProjectiles, Issues);

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
		if (!IsRegisteredOrNone(Character.PassiveBehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("PassiveBehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		SeenIds.Add(Character.Id);
		Snapshot.Characters.Add(Character.Id, Character);
	}
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
		if (FReEchoCsvCardRow* Card = Snapshot.Cards.Find(Effect.CardId))
		{
			Card->Effects.Add(Effect);
		}
		else
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("CardId"), TEXT("Unknown Cards.Id reference"));
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
	bDefaultRegistrationsReady = true;
}

void FReEchoCsvDataRegistry::RegisterBuiltInCsvBehaviors()
{
	RegisterBehaviorId(TEXT("Character.SageBonusChoice"));
	RegisterBehaviorId(TEXT("Character.PoetReactionGrowth"));
	RegisterBehaviorId(TEXT("Character.BraveForge"));
	RegisterBehaviorId(TEXT("Card.StatModifier"));
	RegisterBehaviorId(TEXT("Card.InstantRecovery"));
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
		ReadCharactersTable(DataDirectory, ManifestEntries[CharactersTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReadCharacterAliasesTable(
		    DataDirectory, ManifestEntries[CharacterAliasesTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReadCardsTable(DataDirectory, ManifestEntries[CardsTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReadCardEffectsTable(DataDirectory, ManifestEntries[CardEffectsTableId], *MutableSnapshot, Result.Issues);
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
		FScopeLock Lock(&RegistryCriticalSection);
		PublishedSnapshot = Result.Snapshot;
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
	FScopeLock Lock(&RegistryCriticalSection);
	PublishedSnapshot.Reset();
}
