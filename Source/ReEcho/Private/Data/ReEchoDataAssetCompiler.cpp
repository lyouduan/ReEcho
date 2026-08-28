#include "ReEchoDataAssetCompiler.h"

#include "Cards/ReEchoCardCatalog.h"
#include "Data/ReEchoDataAssets.h"

namespace
{
void AddIssue(TArray<FReEchoCsvIssue>& Issues,
	          const UObject* Asset,
	          const int32 Index,
	          const TCHAR* Field,
	          const FString& Message)
{
	FReEchoCsvIssue& Issue = Issues.AddDefaulted_GetRef();
	Issue.File = Asset ? Asset->GetPathName() : TEXT("ReEcho data catalog");
	Issue.Line = Index + 1;
	Issue.Field = Field;
	Issue.Message = Message;
}

template <typename RowType>
bool AddNamedRows(const UObject* Asset,
	              const TArray<RowType>& Rows,
	              TMap<FName, RowType>& Target,
	              TArray<FName>* Order,
	              TArray<FReEchoCsvIssue>& Issues)
{
	bool bValid = true;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const RowType& Row = Rows[Index];
		if (Row.Id.IsNone())
		{
			AddIssue(Issues, Asset, Index, TEXT("Id"), TEXT("Stable id is required"));
			bValid = false;
			continue;
		}
		if (Target.Contains(Row.Id))
		{
			AddIssue(Issues, Asset, Index, TEXT("Id"), FString::Printf(TEXT("Duplicate id %s"), *Row.Id.ToString()));
			bValid = false;
			continue;
		}
		Target.Add(Row.Id, Row);
		if (Order)
		{
			Order->Add(Row.Id);
		}
	}
	return bValid;
}

EReEchoCardValueOperation ToCardOperation(const EReEchoCsvValueOp Operation)
{
	switch (Operation)
	{
		case EReEchoCsvValueOp::Add: return EReEchoCardValueOperation::Add;
		case EReEchoCsvValueOp::Multiply: return EReEchoCardValueOperation::Multiply;
		case EReEchoCsvValueOp::Override: return EReEchoCardValueOperation::Override;
		default: return EReEchoCardValueOperation::Add;
	}
}

bool CompileCards(const UReEchoCardDataAsset& Asset,
	              FReEchoCsvDataSnapshot& Snapshot,
	              TArray<FReEchoCsvIssue>& Issues)
{
	TArray<FReEchoCardDefinition> Definitions;
	for (const FName CardId : Snapshot.CardOrder)
	{
		const FReEchoCsvCardRow& Row = Snapshot.Cards[CardId];
		FReEchoCardDefinition Definition;
		Definition.Id = Row.Id;
		Definition.Tier = Row.Tier;
		Definition.DisplayName = Row.DisplayName;
		Definition.Description = Row.Description;
		Definition.Tags = Row.Tags;
		Definition.PromotionRoleId = Row.PromotionRoleId;
		Definition.OfferGroup = Row.OfferGroup;
		Definition.StackPolicy = Row.StackPolicy;
		Definition.ConflictPolicy = Row.ConflictPolicy;
		Definition.bEnabled = Row.bEnabled;
		Definition.bOfferable = Row.bOfferable;
		for (const FReEchoCsvCardEffectRow& SourceEffect : Row.Effects)
		{
			FReEchoCardEffectDefinition& Effect = Definition.Effects.AddDefaulted_GetRef();
			Effect.Id = SourceEffect.Id;
			Effect.Order = SourceEffect.Order;
			Effect.Trigger = SourceEffect.Trigger;
			Effect.BehaviorId = SourceEffect.BehaviorId;
			Effect.Target = SourceEffect.Target;
			Effect.Operation = ToCardOperation(SourceEffect.ValueOp);
			Effect.Value = SourceEffect.Value;
			Effect.ParamName = SourceEffect.ParamName;
			Effect.ParamValue = SourceEffect.ParamValue;
		}
		Definitions.Add(MoveTemp(Definition));
	}

	TSharedRef<FReEchoCardCatalog> Catalog = MakeShared<FReEchoCardCatalog>();
	FString Error;
	if (!Catalog->Initialize(Definitions, Asset.DomainRevision, Error))
	{
		AddIssue(Issues, &Asset, 0, TEXT("Cards"), Error);
		return false;
	}
	Snapshot.CardCatalog = Catalog;
	return true;
}

bool ValidateReferences(const UReEchoGameDataCatalog& Catalog,
	                    const FReEchoCsvDataSnapshot& Snapshot,
	                    TArray<FReEchoCsvIssue>& Issues)
{
	bool bValid = true;
	auto Missing = [&](const UObject* Asset, const int32 Index, const TCHAR* Field, const FName Id)
	{
		AddIssue(Issues, Asset, Index, Field, FString::Printf(TEXT("Unknown reference %s"), *Id.ToString()));
		bValid = false;
	};

	for (int32 Index = 0; Index < Catalog.Core->Characters.Num(); ++Index)
	{
		const FReEchoCsvCharacterRow& Row = Catalog.Core->Characters[Index];
		if (!Snapshot.Weapons.Contains(Row.DefaultWeaponId)) Missing(Catalog.Core, Index, TEXT("DefaultWeaponId"), Row.DefaultWeaponId);
	}
	for (int32 Index = 0; Index < Catalog.Core->CharacterAliases.Num(); ++Index)
	{
		const FReEchoCharacterAliasRow& Row = Catalog.Core->CharacterAliases[Index];
		if (!Snapshot.Characters.Contains(Row.CanonicalCharacterId)) Missing(Catalog.Core, Index, TEXT("CanonicalCharacterId"), Row.CanonicalCharacterId);
	}
	for (int32 Index = 0; Index < Catalog.Core->CharacterAbilities.Num(); ++Index)
	{
		const FReEchoCsvCharacterAbilityRow& Row = Catalog.Core->CharacterAbilities[Index];
		if (!Snapshot.Characters.Contains(Row.CharacterId)) Missing(Catalog.Core, Index, TEXT("CharacterId"), Row.CharacterId);
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(Row.BehaviorId)) Missing(Catalog.Core, Index, TEXT("BehaviorId"), Row.BehaviorId);
	}
	for (int32 Index = 0; Index < Catalog.Elements->Reactions.Num(); ++Index)
	{
		const FReEchoCsvReactionRow& Row = Catalog.Elements->Reactions[Index];
		if (!Snapshot.Elements.Contains(Row.TriggerElementId)) Missing(Catalog.Elements, Index, TEXT("TriggerElementId"), Row.TriggerElementId);
		if (!Snapshot.Elements.Contains(Row.AttachmentElementId)) Missing(Catalog.Elements, Index, TEXT("AttachmentElementId"), Row.AttachmentElementId);
		if (!Row.StatusId.IsNone() && !Snapshot.Statuses.Contains(Row.StatusId)) Missing(Catalog.Elements, Index, TEXT("StatusId"), Row.StatusId);
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(Row.BehaviorId)) Missing(Catalog.Elements, Index, TEXT("BehaviorId"), Row.BehaviorId);
		if (!FReEchoCsvDataRegistry::IsFormulaIdRegistered(Row.FormulaId)) Missing(Catalog.Elements, Index, TEXT("FormulaId"), Row.FormulaId);
	}
	for (int32 Index = 0; Index < Catalog.Weapons->Weapons.Num(); ++Index)
	{
		const FReEchoCsvWeaponRow& Row = Catalog.Weapons->Weapons[Index];
		if (!Snapshot.WeaponTypes.Contains(Row.WeaponTypeId)) Missing(Catalog.Weapons, Index, TEXT("WeaponTypeId"), Row.WeaponTypeId);
		if (!FReEchoCsvDataRegistry::IsAttackPatternIdRegistered(Row.AttackPatternId)) Missing(Catalog.Weapons, Index, TEXT("AttackPatternId"), Row.AttackPatternId);
	}
	for (int32 Index = 0; Index < Catalog.Weapons->SlotProfiles.Num(); ++Index)
	{
		const FReEchoCsvSlotProfileRow& Row = Catalog.Weapons->SlotProfiles[Index];
		if (!Snapshot.WeaponTypes.Contains(Row.WeaponTypeId)) Missing(Catalog.Weapons, Index, TEXT("WeaponTypeId"), Row.WeaponTypeId);
		if (!Snapshot.SlotTypes.Contains(Row.SlotTypeId)) Missing(Catalog.Weapons, Index, TEXT("SlotTypeId"), Row.SlotTypeId);
	}
	for (int32 Index = 0; Index < Catalog.Enemies->EnemyCombatStats.Num(); ++Index)
	{
		const FReEchoCsvEnemyCombatStatRow& Row = Catalog.Enemies->EnemyCombatStats[Index];
		if (!Snapshot.Enemies.Contains(Row.EnemyId)) Missing(Catalog.Enemies, Index, TEXT("EnemyId"), Row.EnemyId);
	}
	for (int32 Index = 0; Index < Catalog.Encounters->Encounters.Num(); ++Index)
	{
		const FReEchoCsvEncounterRow& Row = Catalog.Encounters->Encounters[Index];
		if (!Snapshot.Stages.Contains(Row.StageId)) Missing(Catalog.Encounters, Index, TEXT("StageId"), Row.StageId);
	}
	for (int32 Index = 0; Index < Catalog.Encounters->EncounterWaves.Num(); ++Index)
	{
		const FReEchoCsvEncounterWaveRow& Row = Catalog.Encounters->EncounterWaves[Index];
		if (!Snapshot.Encounters.Contains(Row.EncounterId)) Missing(Catalog.Encounters, Index, TEXT("EncounterId"), Row.EncounterId);
		if (!Row.BossEnemyId.IsNone() && !Snapshot.Enemies.Contains(Row.BossEnemyId)) Missing(Catalog.Encounters, Index, TEXT("BossEnemyId"), Row.BossEnemyId);
	}
	for (int32 Index = 0; Index < Catalog.Encounters->SpawnProfiles.Num(); ++Index)
	{
		const FReEchoCsvSpawnProfileRow& Row = Catalog.Encounters->SpawnProfiles[Index];
		if (!Snapshot.Enemies.Contains(Row.EnemyId)) Missing(Catalog.Encounters, Index, TEXT("EnemyId"), Row.EnemyId);
	}
	return bValid;
}
}

FReEchoCsvLoadResult ReEchoDataAssetCompiler::Compile(const UReEchoGameDataCatalog& Catalog)
{
	FReEchoCsvDataRegistry::RegisterBuiltInCsvBehaviors();
	FReEchoCsvLoadResult Result;
	if (!Catalog.Core || !Catalog.Cards || !Catalog.Elements || !Catalog.Weapons || !Catalog.Enemies ||
	    !Catalog.Encounters || !Catalog.Shop)
	{
		AddIssue(Result.Issues, &Catalog, 0, TEXT("DataDomains"), TEXT("Every data domain asset is required"));
		return Result;
	}

	TSharedRef<FReEchoCsvDataSnapshot> Snapshot = MakeShared<FReEchoCsvDataSnapshot>();
	Snapshot->SchemaVersion = FReEchoCsvDataRegistry::SupportedSchemaVersion;
	Snapshot->CardDomainRevision = Catalog.Cards->DomainRevision;
	Snapshot->WeaponDomainRevision = Catalog.Weapons->DomainRevision;

	AddNamedRows(Catalog.Core, Catalog.Core->RuntimeSmokeRows, Snapshot->RuntimeSmokeRows, nullptr, Result.Issues);
	AddNamedRows(Catalog.Core, Catalog.Core->Characters, Snapshot->Characters, nullptr, Result.Issues);
	for (int32 Index = 0; Index < Catalog.Core->CharacterAliases.Num(); ++Index)
	{
		const FReEchoCharacterAliasRow& Row = Catalog.Core->CharacterAliases[Index];
		if (Row.AliasId.IsNone() || Snapshot->CharacterAliases.Contains(Row.AliasId))
		{
			AddIssue(Result.Issues, Catalog.Core, Index, TEXT("AliasId"), TEXT("Alias id is empty or duplicated"));
		}
		else Snapshot->CharacterAliases.Add(Row.AliasId, Row.CanonicalCharacterId);
	}
	AddNamedRows(Catalog.Core, Catalog.Core->CharacterAbilities, Snapshot->CharacterAbilities, &Snapshot->CharacterAbilityOrder, Result.Issues);
	AddNamedRows(Catalog.Core, Catalog.Core->Attributes, Snapshot->Attributes, &Snapshot->AttributeOrder, Result.Issues);
	AddNamedRows(Catalog.Cards, Catalog.Cards->Cards, Snapshot->Cards, &Snapshot->CardOrder, Result.Issues);
	AddNamedRows(Catalog.Elements, Catalog.Elements->Elements, Snapshot->Elements, &Snapshot->ElementOrder, Result.Issues);
	AddNamedRows(Catalog.Elements, Catalog.Elements->Statuses, Snapshot->Statuses, &Snapshot->StatusOrder, Result.Issues);
	AddNamedRows(Catalog.Elements, Catalog.Elements->Reactions, Snapshot->Reactions, &Snapshot->ReactionOrder, Result.Issues);
	AddNamedRows(Catalog.Weapons, Catalog.Weapons->WeaponTypes, Snapshot->WeaponTypes, &Snapshot->WeaponTypeOrder, Result.Issues);
	AddNamedRows(Catalog.Weapons, Catalog.Weapons->Weapons, Snapshot->Weapons, &Snapshot->WeaponOrder, Result.Issues);
	AddNamedRows(Catalog.Weapons, Catalog.Weapons->AttackSteps, Snapshot->AttackSteps, &Snapshot->AttackStepOrder, Result.Issues);
	AddNamedRows(Catalog.Weapons, Catalog.Weapons->SlotTypes, Snapshot->SlotTypes, nullptr, Result.Issues);
	AddNamedRows(Catalog.Weapons, Catalog.Weapons->SlotProfiles, Snapshot->SlotProfiles, nullptr, Result.Issues);
	AddNamedRows(Catalog.Weapons, Catalog.Weapons->Parts, Snapshot->Parts, nullptr, Result.Issues);
	AddNamedRows(Catalog.Enemies, Catalog.Enemies->Enemies, Snapshot->Enemies, &Snapshot->EnemyOrder, Result.Issues);
	AddNamedRows(Catalog.Enemies, Catalog.Enemies->EnemyCombatStats, Snapshot->EnemyCombatStats, &Snapshot->EnemyCombatStatOrder, Result.Issues);
	for (int32 Index = 0; Index < Catalog.Enemies->EnemyShardDrops.Num(); ++Index)
	{
		const FReEchoCsvEnemyShardDropRow& Row = Catalog.Enemies->EnemyShardDrops[Index];
		if (Snapshot->EnemyShardDrops.Contains(Row.EncounterIndex)) AddIssue(Result.Issues, Catalog.Enemies, Index, TEXT("EncounterIndex"), TEXT("Duplicate encounter drop row"));
		else Snapshot->EnemyShardDrops.Add(Row.EncounterIndex, Row);
	}
	AddNamedRows(Catalog.Encounters, Catalog.Encounters->Stages, Snapshot->Stages, &Snapshot->StageOrder, Result.Issues);
	AddNamedRows(Catalog.Encounters, Catalog.Encounters->Encounters, Snapshot->Encounters, &Snapshot->EncounterOrder, Result.Issues);
	AddNamedRows(Catalog.Encounters, Catalog.Encounters->EncounterWaves, Snapshot->EncounterWaves, &Snapshot->EncounterWaveOrder, Result.Issues);
	AddNamedRows(Catalog.Encounters, Catalog.Encounters->SpawnProfiles, Snapshot->SpawnProfiles, &Snapshot->SpawnProfileOrder, Result.Issues);
	AddNamedRows(Catalog.Encounters, Catalog.Encounters->SpawnPolicies, Snapshot->SpawnPolicies, nullptr, Result.Issues);
	for (int32 Index = 0; Index < Catalog.Shop->PriceRanges.Num(); ++Index)
	{
		const FReEchoCsvShopPriceRangeRow& Row = Catalog.Shop->PriceRanges[Index];
		if (Row.PriceCategory.IsNone() || Snapshot->ShopPriceRanges.Contains(Row.PriceCategory)) AddIssue(Result.Issues, Catalog.Shop, Index, TEXT("PriceCategory"), TEXT("Price category is empty or duplicated"));
		else Snapshot->ShopPriceRanges.Add(Row.PriceCategory, Row);
	}
	for (int32 Index = 0; Index < Catalog.Shop->DropLevels.Num(); ++Index)
	{
		const FReEchoCsvShopDropLevelRow& Row = Catalog.Shop->DropLevels[Index];
		if (Snapshot->ShopDropLevels.Contains(Row.EncounterIndex)) AddIssue(Result.Issues, Catalog.Shop, Index, TEXT("EncounterIndex"), TEXT("Duplicate shop drop level"));
		else Snapshot->ShopDropLevels.Add(Row.EncounterIndex, Row);
	}
	for (int32 Index = 0; Index < Catalog.Shop->RefreshRules.Num(); ++Index)
	{
		const FReEchoCsvShopRefreshRuleRow& Row = Catalog.Shop->RefreshRules[Index];
		if (Row.RuleId.IsNone() || Snapshot->ShopRefreshRules.Contains(Row.RuleId)) AddIssue(Result.Issues, Catalog.Shop, Index, TEXT("RuleId"), TEXT("Refresh rule id is empty or duplicated"));
		else Snapshot->ShopRefreshRules.Add(Row.RuleId, Row);
	}

	if (Result.Issues.IsEmpty()) ValidateReferences(Catalog, *Snapshot, Result.Issues);
	if (Result.Issues.IsEmpty()) CompileCards(*Catalog.Cards, *Snapshot, Result.Issues);
	if (Result.Issues.IsEmpty())
	{
		Result.bSuccess = true;
		Result.Snapshot = Snapshot;
	}
	return Result;
}
