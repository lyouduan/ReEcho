#include "ReEchoElementReactionCsvReader.h"

#include "Misc/Paths.h"

namespace ReEchoElementReactionCsv
{
namespace
{
constexpr const TCHAR* ElementsTableId = TEXT("Elements");
constexpr const TCHAR* StatusesTableId = TEXT("Statuses");
constexpr const TCHAR* ReactionsTableId = TEXT("Reactions");
constexpr const TCHAR* BehaviorNone = TEXT("None");
constexpr const TCHAR* StatusNone = TEXT("None");

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

bool ParseElementEnum(const FName ElementId, EReEchoElement& OutElement)
{
	if (ElementId == TEXT("Flame"))
	{
		OutElement = EReEchoElement::Flame;
		return true;
	}
	if (ElementId == TEXT("Lightning"))
	{
		OutElement = EReEchoElement::Lightning;
		return true;
	}
	if (ElementId == TEXT("Grass"))
	{
		OutElement = EReEchoElement::Grass;
		return true;
	}
	if (ElementId == TEXT("Water"))
	{
		OutElement = EReEchoElement::Water;
		return true;
	}
	OutElement = EReEchoElement::None;
	return false;
}

bool ParseElementRole(const FString& Text, EReEchoElementRole& OutRole)
{
	if (Text == TEXT("Trigger"))
	{
		OutRole = EReEchoElementRole::Trigger;
		return true;
	}
	if (Text == TEXT("Attachment"))
	{
		OutRole = EReEchoElementRole::Attachment;
		return true;
	}
	return false;
}

bool IsRegisteredOrNone(const FName BehaviorId)
{
	return BehaviorId == BehaviorNone || FReEchoCsvDataRegistry::IsBehaviorIdRegistered(BehaviorId);
}

bool IsAllowedStatusBehavior(const FName StatusId, const FName BehaviorId)
{
	if (BehaviorId == TEXT("Status.ElementImmunity"))
	{
		return StatusId == TEXT("Z_Elemental_Immunity");
	}
	if (BehaviorId == TEXT("Status.Burn"))
	{
		return StatusId == TEXT("Z_Burn");
	}
	if (BehaviorId == TEXT("Status.Stun"))
	{
		return StatusId == TEXT("Z_Vertigo");
	}
	if (BehaviorId == TEXT("Status.Bleeding"))
	{
		return StatusId == TEXT("Z_Bleeding");
	}
	if (BehaviorId == TEXT("Status.Cursed"))
	{
		return StatusId == TEXT("Z_Cursed");
	}
	return BehaviorId == BehaviorNone;
}

bool IsAllowedReactionBehaviorFormulaPair(const FName BehaviorId, const FName FormulaId)
{
	if (BehaviorId == TEXT("Reaction.Enhance"))
	{
		return FormulaId == TEXT("Element.EnhanceNextReaction");
	}
	if (BehaviorId == TEXT("Reaction.Burn"))
	{
		return FormulaId == TEXT("Element.ElementAttackDot");
	}
	if (BehaviorId == TEXT("Reaction.Vaporize"))
	{
		return FormulaId == TEXT("Element.ElementAttackSquared");
	}
	if (BehaviorId == TEXT("Reaction.Growth"))
	{
		return FormulaId == TEXT("Element.AttachInRadius");
	}
	if (BehaviorId == TEXT("Reaction.Conduct"))
	{
		return FormulaId == TEXT("Element.ChainElementAttack");
	}
	return false;
}

bool ReadElementsTable(const FString& DataDirectory,
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
	                            TEXT("Role"),
	                            TEXT("DisplayName"),
	                            TEXT("Description"),
	                            TEXT("DisplayNameKey"),
	                            TEXT("ColorHex"),
	                            TEXT("VisualKey"),
	                            TEXT("Enabled"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<EReEchoElement> SeenEnums;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvElementRow Element;
		FString RoleText;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Element.Id, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("SourceWorkbookId"), Element.SourceWorkbookId, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Role"), RoleText, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Element.DisplayName, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Description"), Element.Description, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayNameKey"), Element.DisplayNameKey, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("ColorHex"), Element.ColorHex, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("VisualKey"), Element.VisualKey, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Element.bEnabled, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Element.DisabledReason);

		if (!ParseElementEnum(Element.Id, Element.Element))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Unsupported element enum id"));
		}
		if (!ParseElementRole(RoleText, Element.Role))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Role"), TEXT("Role must be Trigger or Attachment"));
		}
		if (!Element.ColorHex.StartsWith(TEXT("#")) || Element.ColorHex.Len() != 7)
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("ColorHex"), TEXT("Color must be #RRGGBB"));
		}
		if (Element.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(Element.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (Element.Element != EReEchoElement::None && SeenEnums.Contains(Element.Element))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate element enum mapping"));
		}
		if (!Element.bEnabled && Element.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(Element.Id);
		SeenEnums.Add(Element.Element);
		Snapshot.ElementOrder.Add(Element.Id);
		Snapshot.Elements.Add(Element.Id, Element);
	}
	return Issues.Num() == 0;
}

bool ReadStatusesTable(const FString& DataDirectory,
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
	                            TEXT("Description"),
	                            TEXT("BehaviorId"),
	                            TEXT("DurationSeconds"),
	                            TEXT("StackPolicy"),
	                            TEXT("RefreshPolicy"),
	                            TEXT("MutexGroup"),
	                            TEXT("Tags"),
	                            TEXT("Enabled"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvStatusRow Status;
		FString Tags;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Status.Id, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Status.DisplayName, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Description"), Status.Description, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BehaviorId"), Status.BehaviorId, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DurationSeconds"), 0.0f, 3600.0f, Status.DurationSeconds, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("StackPolicy"), Status.StackPolicy, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("RefreshPolicy"), Status.RefreshPolicy, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("MutexGroup"), Status.MutexGroup, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Tags"), Tags, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Status.bEnabled, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Status.DisabledReason);
		Status.Tags = ParseNameList(Tags);

		if (SeenIds.Contains(Status.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!IsRegisteredOrNone(Status.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!IsAllowedStatusBehavior(Status.Id, Status.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("StatusId/BehaviorId combination is invalid"));
		}
		if (Status.bEnabled && Status.BehaviorId == BehaviorNone)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Enabled status needs behavior"));
		}
		if (!Status.bEnabled && Status.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}
		SeenIds.Add(Status.Id);
		Snapshot.StatusOrder.Add(Status.Id);
		Snapshot.Statuses.Add(Status.Id, Status);
	}
	return Issues.Num() == 0;
}

bool ReadReactionsTable(const FString& DataDirectory,
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
	                            TEXT("Description"),
	                            TEXT("TriggerElementId"),
	                            TEXT("AttachmentElementId"),
	                            TEXT("BehaviorId"),
	                            TEXT("FormulaId"),
	                            TEXT("DamageMultiplier"),
	                            TEXT("DamageIncrease"),
	                            TEXT("RadiusCm"),
	                            TEXT("StatusId"),
	                            TEXT("StatusDurationSeconds"),
	                            TEXT("EnhancementMultiplier"),
	                            TEXT("CanCrit"),
	                            TEXT("AffectedByEchoEfficiency"),
	                            TEXT("ClearsAttachment"),
	                            TEXT("Enabled"),
	                            TEXT("DisabledReason")},
	                           Issues);

	TSet<FName> SeenIds;
	TSet<FString> SeenOrderedPairs;
	for (const ReEchoCsv::FRow& Row : Table.Rows)
	{
		FReEchoCsvReactionRow Reaction;
		ReEchoCsv::RequireStableId(Table, Row, TEXT("Id"), Reaction.Id, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("DisplayName"), Reaction.DisplayName, Issues);
		ReEchoCsv::RequireCell(Table, Row, TEXT("Description"), Reaction.Description, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("TriggerElementId"), Reaction.TriggerElementId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("AttachmentElementId"), Reaction.AttachmentElementId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("BehaviorId"), Reaction.BehaviorId, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("FormulaId"), Reaction.FormulaId, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DamageMultiplier"), 0.0f, 100.0f, Reaction.DamageMultiplier, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("DamageIncrease"), 0.0f, 100.0f, Reaction.DamageIncrease, Issues);
		ReEchoCsv::RequireFloat(Table, Row, TEXT("RadiusCm"), 0.0f, 100000.0f, Reaction.RadiusCm, Issues);
		ReEchoCsv::RequireStableId(Table, Row, TEXT("StatusId"), Reaction.StatusId, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("StatusDurationSeconds"), 0.0f, 3600.0f, Reaction.StatusDurationSeconds, Issues);
		ReEchoCsv::RequireFloat(
		    Table, Row, TEXT("EnhancementMultiplier"), 1.0f, 100.0f, Reaction.EnhancementMultiplier, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("CanCrit"), Reaction.bCanCrit, Issues);
		ReEchoCsv::RequireBool(
		    Table, Row, TEXT("AffectedByEchoEfficiency"), Reaction.bAffectedByEchoEfficiency, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("ClearsAttachment"), Reaction.bClearsAttachment, Issues);
		ReEchoCsv::RequireBool(Table, Row, TEXT("Enabled"), Reaction.bEnabled, Issues);
		ReEchoCsv::ReadOptionalCell(Row, TEXT("DisabledReason"), Reaction.DisabledReason);

		if (SeenIds.Contains(Reaction.Id))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		const FString PairKey = FString::Printf(
		    TEXT("%s>%s"), *Reaction.TriggerElementId.ToString(), *Reaction.AttachmentElementId.ToString());
		if (SeenOrderedPairs.Contains(PairKey))
		{
			ReEchoCsv::AddIssue(Issues, Table.File, Row.Line, TEXT("TriggerElementId"), TEXT("Duplicate ordered pair"));
		}
		SeenOrderedPairs.Add(PairKey);

		const FReEchoCsvElementRow* Trigger = Snapshot.Elements.Find(Reaction.TriggerElementId);
		if (!Trigger)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("TriggerElementId"), TEXT("Unknown Elements.Id reference"));
		}
		const FReEchoCsvElementRow* Attachment = Snapshot.Elements.Find(Reaction.AttachmentElementId);
		if (!Attachment)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("AttachmentElementId"), TEXT("Unknown Elements.Id reference"));
		}
		if (!IsRegisteredOrNone(Reaction.BehaviorId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!FReEchoCsvDataRegistry::IsFormulaIdRegistered(Reaction.FormulaId))
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("FormulaId"), TEXT("Unknown registered C++ formula id"));
		}
		if (!IsAllowedReactionBehaviorFormulaPair(Reaction.BehaviorId, Reaction.FormulaId))
		{
			ReEchoCsv::AddIssue(Issues,
			                    Table.File,
			                    Row.Line,
			                    TEXT("FormulaId"),
			                    TEXT("ReactionBehaviorId/FormulaId combination is invalid"));
		}
		if (Reaction.StatusId != StatusNone)
		{
			const FReEchoCsvStatusRow* Status = Snapshot.Statuses.Find(Reaction.StatusId);
			if (!Status)
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, Row.Line, TEXT("StatusId"), TEXT("Unknown Statuses.Id reference"));
			}
			else if (!Status->bEnabled)
			{
				ReEchoCsv::AddIssue(
				    Issues, Table.File, Row.Line, TEXT("StatusId"), TEXT("Reaction references disabled status"));
			}
		}
		if (Reaction.bEnabled && Reaction.BehaviorId == BehaviorNone)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Enabled reaction needs behavior"));
		}
		if (Reaction.bCanCrit)
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("CanCrit"), TEXT("Element reaction crit is not supported yet"));
		}
		if (!Reaction.bEnabled && Reaction.DisabledReason.IsEmpty())
		{
			ReEchoCsv::AddIssue(
			    Issues, Table.File, Row.Line, TEXT("DisabledReason"), TEXT("Disabled source row requires a reason"));
		}

		SeenIds.Add(Reaction.Id);
		Snapshot.ReactionOrder.Add(Reaction.Id);
		Snapshot.Reactions.Add(Reaction.Id, Reaction);
	}
	return Issues.Num() == 0;
}
}

bool ReadTables(const FString& DataDirectory,
                const TMap<FString, ReEchoCsv::FManifestEntry>& ManifestEntries,
                FReEchoCsvDataSnapshot& Snapshot,
                TArray<FReEchoCsvIssue>& Issues)
{
	ReadElementsTable(DataDirectory, ManifestEntries[ElementsTableId], Snapshot, Issues);
	if (Issues.Num() == 0)
	{
		ReadStatusesTable(DataDirectory, ManifestEntries[StatusesTableId], Snapshot, Issues);
	}
	if (Issues.Num() == 0)
	{
		ReadReactionsTable(DataDirectory, ManifestEntries[ReactionsTableId], Snapshot, Issues);
	}
	return Issues.Num() == 0;
}
}
