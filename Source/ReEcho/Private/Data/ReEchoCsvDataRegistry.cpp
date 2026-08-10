#include "Data/ReEchoCsvDataRegistry.h"

#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "ReEcho.h"
#include "ReEchoCharacterBuildCsvReader.h"
#include "ReEchoCsvDataReader.h"
#include "ReEchoElementReactionCsvReader.h"

namespace
{
constexpr const TCHAR* RuntimeSmokeTableId = TEXT("RuntimeSmoke");
constexpr const TCHAR* RuntimeSmokeEffectsTableId = TEXT("RuntimeSmokeEffects");
constexpr const TCHAR* CharactersTableId = TEXT("Characters");
constexpr const TCHAR* CharacterAliasesTableId = TEXT("CharacterAliases");
constexpr const TCHAR* CardsTableId = TEXT("Cards");
constexpr const TCHAR* CardEffectsTableId = TEXT("CardEffects");
constexpr const TCHAR* ElementsTableId = TEXT("Elements");
constexpr const TCHAR* StatusesTableId = TEXT("Statuses");
constexpr const TCHAR* ReactionsTableId = TEXT("Reactions");

constexpr const TCHAR* BehaviorNone = TEXT("None");
constexpr const TCHAR* DefaultBehaviorId = TEXT("RuntimeSmoke.LogValue");
constexpr const TCHAR* DefaultEffectKind = TEXT("ScalarModifier");
constexpr const TCHAR* StatModifierEffectKind = TEXT("StatModifier");
constexpr const TCHAR* InstantRecoveryEffectKind = TEXT("InstantRecovery");
constexpr const TCHAR* ElementReactionEffectKind = TEXT("ElementReaction");

FCriticalSection RegistryCriticalSection;
TSharedPtr<const FReEchoCsvDataSnapshot> PublishedSnapshot;
TSet<FName> RegisteredBehaviorIds;
TSet<FName> RegisteredEffectKinds;
TSet<FName> RegisteredFormulaIds;
bool bDefaultRegistrationsReady = false;

TArray<FString> GetRequiredTableIds()
{
	return {RuntimeSmokeTableId,
	        RuntimeSmokeEffectsTableId,
	        CharactersTableId,
	        CharacterAliasesTableId,
	        CardsTableId,
	        CardEffectsTableId,
	        ElementsTableId,
	        StatusesTableId,
	        ReactionsTableId};
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
	RegisteredEffectKinds.Add(FName(ElementReactionEffectKind));
	bDefaultRegistrationsReady = true;
}

void FReEchoCsvDataRegistry::RegisterBuiltInCsvBehaviors()
{
	RegisterBehaviorId(TEXT("Character.SageBonusChoice"));
	RegisterBehaviorId(TEXT("Character.PoetReactionGrowth"));
	RegisterBehaviorId(TEXT("Character.BraveForge"));
	RegisterBehaviorId(TEXT("Card.StatModifier"));
	RegisterBehaviorId(TEXT("Card.InstantRecovery"));
	RegisterBehaviorId(TEXT("Status.ElementImmunity"));
	RegisterBehaviorId(TEXT("Status.Burn"));
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
		ReEchoElementReactionCsv::ReadTables(DataDirectory, ManifestEntries, *MutableSnapshot, Result.Issues);
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
