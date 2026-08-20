#include "Run/ReEchoRunSubsystem.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Core/ReEchoBalanceSettings.h"
#include "ReEcho.h"
#include "Run/ReEchoCharacterPromotion.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoShopCatalog.h"
#include "Cards/ReEchoCardRuntime.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr const TCHAR* TraitOfferGroup = TEXT("Trait");
constexpr const TCHAR* ForgeOfferGroup = TEXT("Forge");
const FName AnyWeaponTypeId = TEXT("Any");

const FString RunSaveSlot = TEXT("ReEchoRun");
constexpr int32 RunSaveUserIndex = 0;

bool IsValidResumableSave(const UReEchoRunSaveGame& SaveGame)
{
	// v4/v5 archives stay resumable because RestoreSaveSnapshot migrates them forward.
	if (SaveGame.SaveVersion < UReEchoRunSaveGame::MinimumSupportedSaveVersion ||
	    SaveGame.SaveVersion > UReEchoRunSaveGame::CurrentSaveVersion ||
	    SaveGame.SavedPhase == EReEchoRunPhase::Summary || SaveGame.SavedPhase == EReEchoRunPhase::Failed)
	{
		return false;
	}
	if (SaveGame.SavedPhase == EReEchoRunPhase::Encounter)
	{
		return SaveGame.EncounterRuntimeState.bValid && SaveGame.EncounterRuntimeState.PlayerHealth > 0.0f &&
		       SaveGame.EncounterRuntimeState.EncounterTime >= 0.0f;
	}
	return true;
}

FString GetBuildConfigurationError(const FReEchoBuildSnapshot& Build)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return TEXT("CSV snapshot is unavailable");
	}
	return ReEchoWeaponRuntime::GetBuildConfigurationError(*Snapshot, Build);
}

bool ValidateRecordingAgainstSnapshot(const FReEchoCsvDataSnapshot& Snapshot,
                                      const FReEchoRecording& Recording,
                                      FString& OutError)
{
	OutError = ReEchoWeaponRuntime::GetBuildConfigurationError(Snapshot, Recording.BuildSnapshot);
	if (!OutError.IsEmpty())
	{
		return false;
	}
	return true;
}

void RequireConfiguredBuild(const FReEchoBuildSnapshot& Build, const TCHAR* Context)
{
	const FString Error = GetBuildConfigurationError(Build);
	if (!Error.IsEmpty())
	{
		UE_LOG(LogReEcho, Fatal, TEXT("%s: %s"), Context, *Error);
	}
}

FReEchoTraitCardOffer MakeTraitOffer(const FReEchoCsvCardRow& Card)
{
	FReEchoTraitCardOffer Offer;
	Offer.CardId = Card.Id;
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.Description = FText::FromString(Card.Description);
	Offer.Tags = Card.Tags;
	return Offer;
}

FReEchoTraitCardOffer MakeTraitOffer(const FReEchoCardDefinition& Card)
{
	FReEchoTraitCardOffer Offer;
	Offer.CardId = Card.Id;
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.Description = FText::FromString(Card.Description);
	Offer.Tags = Card.Tags;
	return Offer;
}

bool IsPartCompatibleWithWeapon(const FReEchoCsvDataSnapshot& Snapshot,
                                const FReEchoCsvPartRow& Part,
                                const FReEchoCsvWeaponRow& Weapon)
{
	return Part.bEnabled && (Part.WeaponTypeId == AnyWeaponTypeId || Part.WeaponTypeId == Weapon.WeaponTypeId) &&
	       ReEchoWeaponRuntime::GetEffectiveSlotCapacity(
	           Snapshot, FReEchoBuildSnapshot{}, Weapon.WeaponTypeId, Part.SlotTypeId) > 0;
}

FReEchoShopOffer MakeWeaponPartOffer(const FReEchoCsvPartRow& Part)
{
	FReEchoShopOffer Offer;
	Offer.ItemId = Part.PartId;
	Offer.DisplayName = FText::FromString(Part.DisplayName);
	Offer.EffectText = FText::FromString(Part.Description);
	Offer.Price = Part.ShopPrice;
	Offer.Type = EReEchoShopOfferType::WeaponPart;
	Offer.ContentId = Part.PartId;
	Offer.SlotTypeId = Part.SlotTypeId;
	return Offer;
}

FName MakeShopCardOfferId(const int32 RefreshSequence, const FName CardId)
{
	return FName(*FString::Printf(TEXT("SHOP_CARD_%d_%s"), RefreshSequence, *CardId.ToString()));
}

FReEchoShopOffer MakeBuildCardOffer(const FReEchoCardDefinition& Card, const int32 RefreshSequence)
{
	FReEchoShopOffer Offer;
	Offer.ItemId = MakeShopCardOfferId(RefreshSequence, Card.Id);
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.EffectText = FText::FromString(Card.Description);
	Offer.Tier = FMath::Clamp(Card.Tier, 1, 3);
	Offer.Price = Offer.Tier * 10;
	Offer.Type = EReEchoShopOfferType::BuildCard;
	Offer.ContentId = Card.Id;
	return Offer;
}

int32 BuildShopOfferSeed(const FName WeaponId, const int32 EncounterIndex, const int32 RefreshSequence)
{
	uint32 Seed = HashCombine(GetTypeHash(WeaponId), GetTypeHash(EncounterIndex));
	Seed = HashCombine(Seed, GetTypeHash(RefreshSequence));
	return static_cast<int32>(Seed);
}

bool TryNormalizeOwnedParts(const FReEchoCsvDataSnapshot& Snapshot,
                            const TArray<FName>& SavedOwnedParts,
                            const TArray<FReEchoEquippedPartSnapshot>& EquippedParts,
                            TArray<FName>& OutOwnedParts)
{
	OutOwnedParts.Reset();
	TSet<FName> Seen;
	auto AddPart = [&](const FName PartId)
	{
		const FReEchoCsvPartRow* Part = Snapshot.Parts.Find(PartId);
		if (!Part || !Part->bEnabled || Part->PartId != PartId)
		{
			return false;
		}
		if (!Seen.Contains(PartId))
		{
			Seen.Add(PartId);
			OutOwnedParts.Add(PartId);
		}
		return true;
	};
	for (const FName PartId : SavedOwnedParts)
	{
		if (!AddPart(PartId))
		{
			return false;
		}
	}
	for (const FReEchoEquippedPartSnapshot& Equipped : EquippedParts)
	{
		if (!AddPart(Equipped.PartId))
		{
			return false;
		}
	}
	return true;
}

TArray<FReEchoCsvCardRow> GetOfferCatalog(const FName OfferGroup)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	return Snapshot.IsValid() ? Snapshot->GetOfferableCards(OfferGroup) : TArray<FReEchoCsvCardRow>();
}

int32 GetTraitStackCount(const TArray<FName>& OwnedCards, const FName CardId)
{
	int32 Count = 0;
	for (const FName OwnedCardId : OwnedCards)
	{
		if (OwnedCardId == CardId)
		{
			++Count;
		}
	}
	return Count;
}

template <typename T> void ShuffleOffers(TArray<T>& Offers, FRandomStream& Random)
{
	for (int32 Index = Offers.Num() - 1; Index > 0; --Index)
	{
		Offers.Swap(Index, Random.RandRange(0, Index));
	}
}

int32 BuildTraitOfferSeed(const int32 RunSeed, const int32 EncounterIndex, const TArray<FName>& OwnedCards)
{
	uint32 Seed = HashCombine(GetTypeHash(RunSeed), GetTypeHash(EncounterIndex));
	Seed = HashCombine(Seed, GetTypeHash(OwnedCards.Num()));
	for (const FName CardId : OwnedCards)
	{
		Seed = HashCombine(Seed, GetTypeHash(CardId));
	}
	return static_cast<int32>(Seed);
}

int32 MakeNewTraitOfferSeed()
{
	const int32 Seed = static_cast<int32>(GetTypeHash(FGuid::NewGuid()));
	return Seed != 0 ? Seed : 1;
}

int32 MigrateLegacyTraitOfferSeed(const UReEchoRunSaveGame& SaveGame)
{
	uint32 Seed = HashCombine(GetTypeHash(SaveGame.CurrentBuild.CharacterId),
	                          GetTypeHash(SaveGame.CurrentBuild.WeaponId));
	Seed = HashCombine(Seed, GetTypeHash(SaveGame.EncounterIndex));
	Seed = HashCombine(Seed, 0x52454348u); // "RECH": stable migration salt.
	return Seed != 0 ? static_cast<int32>(Seed) : 1;
}

float ApplyValueOperation(const float CurrentValue, const EReEchoCsvValueOp ValueOp, const float Value)
{
	switch (ValueOp)
	{
		case EReEchoCsvValueOp::Add:
			return CurrentValue + Value;
		case EReEchoCsvValueOp::Multiply:
			return CurrentValue * Value;
		case EReEchoCsvValueOp::Override:
			return Value;
		default:
			return CurrentValue;
	}
}

bool ApplyStatEffect(FReEchoStatBlock& Stats, const FReEchoCsvCardEffectRow& Effect)
{
	auto ApplyFloat = [&](float& Target)
	{
		Target = ApplyValueOperation(Target, Effect.ValueOp, Effect.Value);
	};

	if (Effect.Target == TEXT("HpMax"))
	{
		ApplyFloat(Stats.HpMax);
		Stats.HpMax = FMath::Max(1.0f, Stats.HpMax);
		Stats.HpPoint = FMath::Min(Stats.HpPoint, Stats.HpMax);
		return true;
	}
	if (Effect.Target == TEXT("HpPoint"))
	{
		ApplyFloat(Stats.HpPoint);
		Stats.HpPoint = FMath::Clamp(Stats.HpPoint, 0.0f, Stats.HpMax);
		return true;
	}
	if (Effect.Target == TEXT("PhysicalAttack"))
	{
		ApplyFloat(Stats.PhysicalAttack);
		Stats.PhysicalAttack = FMath::Max(0.0f, Stats.PhysicalAttack);
		return true;
	}
	if (Effect.Target == TEXT("ElementalAttack"))
	{
		ApplyFloat(Stats.ElementalAttack);
		Stats.ElementalAttack = FMath::Max(0.0f, Stats.ElementalAttack);
		return true;
	}
	if (Effect.Target == TEXT("Block"))
	{
		const float NewValue = ApplyValueOperation(static_cast<float>(Stats.Block), Effect.ValueOp, Effect.Value);
		Stats.Block = FMath::Max(0, FMath::RoundToInt(NewValue));
		return true;
	}
	if (Effect.Target == TEXT("AttackSpeed"))
	{
		ApplyFloat(Stats.AttackSpeed);
		Stats.AttackSpeed = FMath::Max(0.1f, Stats.AttackSpeed);
		return true;
	}
	if (Effect.Target == TEXT("MovementSpeed"))
	{
		ApplyFloat(Stats.MovementSpeed);
		Stats.MovementSpeed = FMath::Max(0.1f, Stats.MovementSpeed);
		return true;
	}
	if (Effect.Target == TEXT("EchoEfficiency"))
	{
		ApplyFloat(Stats.EchoEfficiency);
		Stats.EchoEfficiency = FMath::Max(0.0f, Stats.EchoEfficiency);
		return true;
	}
	return false;
}

bool ApplyCardEffects(const FReEchoCsvCardRow& Card, FReEchoBuildSnapshot& Build)
{
	for (const FReEchoCsvCardEffectRow& Effect : Card.Effects)
	{
		if (Effect.Trigger != TEXT("OnApply"))
		{
			return false;
		}
		if (Effect.EffectKind == TEXT("StatModifier") || Effect.EffectKind == TEXT("InstantRecovery"))
		{
			if (!ApplyStatEffect(Build.Stats, Effect))
			{
				return false;
			}
			continue;
		}
		return false;
	}
	return true;
}

bool TryNormalizeEquipmentBuild(const FReEchoCsvDataSnapshot& Snapshot,
                                const FReEchoBuildSnapshot& Build,
                                FReEchoBuildSnapshot& OutBuild)
{
	TArray<FName> PartIds;
	for (const FReEchoEquippedPartSnapshot& Part : Build.EquippedParts)
	{
		PartIds.Add(Part.PartId);
	}
	FString Error;
	return ReEchoWeaponRuntime::TryEquipParts(Snapshot, Build, PartIds, OutBuild, Error);
}

TArray<FName> GetEquippedPartIds(const FReEchoBuildSnapshot& Build)
{
	TArray<FName> PartIds;
	for (const FReEchoEquippedPartSnapshot& Part : Build.EquippedParts)
	{
		PartIds.Add(Part.PartId);
	}
	return PartIds;
}

bool TryMutateAuthoritativeBuild(const FReEchoCsvDataSnapshot& Snapshot,
                                 const FReEchoBuildSnapshot& Build,
                                 TFunctionRef<bool(FReEchoBuildSnapshot&)> Mutator,
                                 FReEchoBuildSnapshot& OutBuild)
{
	FString Error;
	FReEchoBuildSnapshot BaseBuild;
	if (!ReEchoWeaponRuntime::TryGetEquipmentBaseBuild(Build, BaseBuild, Error) || !Mutator(BaseBuild))
	{
		return false;
	}

	BaseBuild.EquipmentBaseStats = BaseBuild.Stats;
	BaseBuild.EquipmentBaseRuleFlags = BaseBuild.RuleFlags;
	BaseBuild.bHasEquipmentBase = true;
	if (Build.WeaponDomainRevision.IsEmpty() && Build.EquippedParts.IsEmpty())
	{
		OutBuild = BaseBuild;
		return true;
	}
	return ReEchoWeaponRuntime::TryEquipParts(Snapshot, BaseBuild, GetEquippedPartIds(Build), OutBuild, Error);
}

bool CurrentCharacterHasPassive(const FReEchoBuildSnapshot& Build, const FName PassiveBehaviorId)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvCharacterRow* Character = Snapshot.IsValid() ? Snapshot->FindCharacter(Build.CharacterId) : nullptr;
	return Character && Character->PassiveBehaviorId == PassiveBehaviorId;
}

FReEchoStoredEchoSummary MakeStoredEchoSummary(const FReEchoRecording& Recording, const bool bSelected)
{
	FReEchoStoredEchoSummary Summary;
	Summary.RecordingId = Recording.Id;
	Summary.EncounterIndex = Recording.EncounterIndex;
	Summary.Duration = Recording.Duration;
	Summary.MapId = Recording.MapId;
	Summary.CharacterId = Recording.BuildSnapshot.CharacterId;
	Summary.WeaponId = Recording.BuildSnapshot.WeaponId;
	Summary.bSelectedForNextEncounter = bSelected;
	return Summary;
}

/** Save-version independent echo storage payload produced by restore and by v4 migration. */
struct FReEchoEchoStorageRestoreState
{
	bool bHasPendingRecording = false;
	FReEchoRecording PendingRecording;
	bool bHasLatestCompletedRecording = false;
	FReEchoRecording LatestCompletedRecording;
	bool bHasPreviousCompletedRecording = false;
	FReEchoRecording PreviousCompletedRecording;
	TArray<FReEchoRecording> StoredEchoes;
	TArray<FGuid> SelectedReplayIds;
	int32 StorageCapacity = ReEchoEchoStorage::DefaultStorageCapacity;
	int32 SpecificReplayLimit = ReEchoEchoStorage::SpecificReplayUnavailable;
};

bool ContainsRecordingId(const TArray<FReEchoRecording>& Recordings, const FGuid& RecordingId)
{
	return Recordings.ContainsByPredicate(
	    [&RecordingId](const FReEchoRecording& Candidate)
	    {
		    return Candidate.Id == RecordingId;
	    });
}

bool MigrateBuildState(const int32 SaveVersion, const FReEchoCsvDataSnapshot& Snapshot, FReEchoBuildSnapshot& Build)
{
	const FName CanonicalCharacterId = Snapshot.ResolveCharacterId(Build.CharacterId);
	if (!Snapshot.FindCharacter(CanonicalCharacterId))
	{
		return false;
	}
	if (SaveVersion < 11)
	{
		Build.CharacterId = CanonicalCharacterId;
	}
	else if (CanonicalCharacterId != Build.CharacterId)
	{
		return false;
	}

	if (FString* BaseCharacterId = Build.RuleFlags.Find(TEXT("BaseCharacterId")))
	{
		const FName CanonicalBaseCharacterId = Snapshot.ResolveCharacterId(FName(**BaseCharacterId));
		if (!Snapshot.FindCharacter(CanonicalBaseCharacterId))
		{
			return false;
		}
		if (SaveVersion < 11)
		{
			*BaseCharacterId = CanonicalBaseCharacterId.ToString();
		}
		else if (CanonicalBaseCharacterId.ToString() != *BaseCharacterId)
		{
			return false;
		}
	}

	if (!Snapshot.CardCatalog.IsValid())
	{
		return false;
	}
	if (SaveVersion >= 9)
	{
		if (!Build.Cards.IsEmpty() || Build.CardState.DomainRevision != Snapshot.CardDomainRevision ||
		    Build.CardState.Runtime.RandomSequence < 0 || Build.CardState.Runtime.PreventedDamageCount < 0 ||
		    Build.CardState.Runtime.HuntKillCount < 0 || Build.CardState.Runtime.ReactionCount < 0 ||
		    Build.CardState.Runtime.EchoKillProgress < 0 || Build.CardState.Runtime.PlayerKillProgress < 0 ||
		    Build.CardState.Runtime.FreeShopRefreshes < 0 ||
		    static_cast<uint8>(Build.CardState.Runtime.EconomyPenalty) >
		        static_cast<uint8>(EReEchoCardEconomyPenalty::NoEnemyShardDrops) ||
		    (Build.CardState.Runtime.bHasAnchorRecording && !Build.CardState.Runtime.AnchorRecordingId.IsValid()))
		{
			return false;
		}
		TSet<FName> UniqueCards;
		for (const FName CardId : Build.CardState.OwnedCardIds)
		{
			const FReEchoCardDefinition* Card = Snapshot.CardCatalog->Find(CardId);
			if (!Card || !Card->bEnabled || Card->OfferGroup != TraitOfferGroup ||
			    (Card->StackPolicy == TEXT("Unique") && UniqueCards.Contains(CardId)))
			{
				return false;
			}
			UniqueCards.Add(CardId);
		}
		return true;
	}

	Build.CardState = {};
	Build.CardState.DomainRevision = Snapshot.CardDomainRevision;
	for (const FName LegacyId : Build.Cards)
	{
		FName MigratedId = LegacyId;
		if (LegacyId == TEXT("G_1_03"))
		{
			continue; // v8 emergency block has no equivalent and must not become physical attack.
		}
		if (LegacyId == TEXT("G_1_04"))
		{
			MigratedId = TEXT("G_1_03");
		}
		else if (LegacyId == TEXT("G_1_05"))
		{
			MigratedId = TEXT("G_1_04");
		}
		else if (LegacyId == TEXT("G_1_08"))
		{
			MigratedId = TEXT("G_1_07");
		}
		if (Snapshot.CardCatalog->Find(MigratedId))
		{
			Build.CardState.OwnedCardIds.Add(MigratedId);
		}
	}
	Build.Cards.Reset();
	return true;
}

/**
 * v4 archives only carried a rolling RecordingHistory plus one AnchorId.
 * The newest history entry becomes the rolling latest echo, and a resolvable anchor is promoted into
 * a single stored + selected echo, i.e. specific single replay. An anchor that is invalid or whose
 * recording is missing normalizes deterministically to "specific replay not unlocked"; migration
 * never substitutes a different encounter for the requested anchor.
 */
void MigrateV4EchoStorage(const UReEchoRunSaveGame& SaveGame, FReEchoEchoStorageRestoreState& OutState)
{
	OutState = FReEchoEchoStorageRestoreState{};
	if (SaveGame.RecordingHistory.Num() > 0 && SaveGame.RecordingHistory[0].Id.IsValid())
	{
		OutState.LatestCompletedRecording = SaveGame.RecordingHistory[0];
		OutState.bHasLatestCompletedRecording = true;
	}
	if (SaveGame.RecordingHistory.Num() > 1 && SaveGame.RecordingHistory[1].Id.IsValid())
	{
		OutState.PreviousCompletedRecording = SaveGame.RecordingHistory[1];
		OutState.bHasPreviousCompletedRecording = true;
	}
	if (!SaveGame.AnchorId.IsValid())
	{
		return;
	}
	const FReEchoRecording* Anchor = SaveGame.RecordingHistory.FindByPredicate(
	    [&SaveGame](const FReEchoRecording& Candidate)
	    {
		    return Candidate.Id == SaveGame.AnchorId;
	    });
	if (!Anchor)
	{
		return;
	}
	OutState.StoredEchoes.Add(*Anchor);
	OutState.SelectedReplayIds.Add(SaveGame.AnchorId);
	OutState.SpecificReplayLimit = 1;
}

/** Reads the v5 layout, clamping capabilities and dropping invalid or duplicated stored ids. */
void ReadV5EchoStorage(const UReEchoRunSaveGame& SaveGame, FReEchoEchoStorageRestoreState& OutState)
{
	OutState = FReEchoEchoStorageRestoreState{};
	OutState.StorageCapacity = FMath::Clamp(SaveGame.StorageCapacity, 0, ReEchoEchoStorage::MaxStorageCapacity);
	OutState.SpecificReplayLimit =
	    FMath::Clamp(SaveGame.SpecificReplayLimit, 0, ReEchoEchoStorage::MaxSpecificReplayLimit);
	if (SaveGame.bHasPendingRecording && SaveGame.PendingRecording.Id.IsValid())
	{
		OutState.PendingRecording = SaveGame.PendingRecording;
		OutState.bHasPendingRecording = true;
	}
	if (SaveGame.bHasLatestCompletedRecording && SaveGame.LatestCompletedRecording.Id.IsValid())
	{
		OutState.LatestCompletedRecording = SaveGame.LatestCompletedRecording;
		OutState.bHasLatestCompletedRecording = true;
	}
	if (SaveGame.bHasPreviousCompletedRecording && SaveGame.PreviousCompletedRecording.Id.IsValid())
	{
		OutState.PreviousCompletedRecording = SaveGame.PreviousCompletedRecording;
		OutState.bHasPreviousCompletedRecording = true;
	}
	for (const FReEchoRecording& Stored : SaveGame.StoredEchoes)
	{
		if (OutState.StoredEchoes.Num() >= OutState.StorageCapacity)
		{
			break;
		}
		if (!Stored.Id.IsValid() || ContainsRecordingId(OutState.StoredEchoes, Stored.Id))
		{
			continue;
		}
		OutState.StoredEchoes.Add(Stored);
	}
	OutState.SelectedReplayIds = SaveGame.SelectedReplayIds;
}

/** Fails the whole restore when a recording's build no longer resolves against current CSV data. */
bool TryNormalizeRestoredRecording(const FReEchoCsvDataSnapshot& Snapshot, FReEchoRecording& Recording)
{
	FString RecordingError;
	if (!ValidateRecordingAgainstSnapshot(Snapshot, Recording, RecordingError))
	{
		return false;
	}
	return TryNormalizeEquipmentBuild(Snapshot, Recording.BuildSnapshot, Recording.BuildSnapshot);
}
}

FReEchoStartRunResolveResult ReEchoRunData::ResolveStartingBuildFromSnapshot(const FReEchoCsvDataSnapshot* Snapshot,
                                                                             const FName CharacterId,
                                                                             const FName WeaponId)
{
	FReEchoStartRunResolveResult Result;
	if (!Snapshot)
	{
		Result.Error = FString::Printf(TEXT("Cannot start run for CharacterId '%s': CSV snapshot is unavailable"),
		                               *CharacterId.ToString());
		return Result;
	}

	const FReEchoCsvCharacterRow* Character = Snapshot->FindCharacter(CharacterId);
	const FName RequestedWeaponId = WeaponId.IsNone() && Character ? Character->DefaultWeaponId : WeaponId;
	const FReEchoCsvWeaponRow* Weapon = Snapshot->FindWeapon(RequestedWeaponId);
	if (!Character)
	{
		Result.Error = FString::Printf(TEXT("Cannot start run for CharacterId '%s': character was not found"),
		                               *CharacterId.ToString());
		return Result;
	}
	if (!Character->bEnabled)
	{
		Result.Error = FString::Printf(TEXT("Cannot start run for CharacterId '%s': character is disabled"),
		                               *CharacterId.ToString());
		return Result;
	}
	if (!Weapon)
	{
		Result.Error = FString::Printf(TEXT("Cannot start run with WeaponId '%s': weapon was not found"),
		                               *RequestedWeaponId.ToString());
		return Result;
	}
	if (!Weapon->bEnabled)
	{
		Result.Error = FString::Printf(TEXT("Cannot start run with WeaponId '%s': weapon is disabled"),
		                               *RequestedWeaponId.ToString());
		return Result;
	}

	Result.Build.CharacterId = Character->Id;
	Result.Build.WeaponId = Weapon->Id;
	Result.Build.WeaponDataRevision = Weapon->DataRevision;
	Result.Build.WeaponDomainRevision = Snapshot->WeaponDomainRevision;
	Result.Build.CardState.DomainRevision = Snapshot->CardDomainRevision;
	Result.Build.Stats = Character->BaseStats;
	Result.Build.Stats.RoleId = Character->RoleId == TEXT("None") ? NAME_None : Character->RoleId;
	Result.Build.RuleFlags.Add(TEXT("BaseCharacterId"), Character->Id.ToString());
	Result.Build.EquipmentBaseStats = Result.Build.Stats;
	Result.Build.EquipmentBaseRuleFlags = Result.Build.RuleFlags;
	Result.Build.bHasEquipmentBase = true;
	Result.bSuccess = true;
	return Result;
}

FReEchoStartRunResolveResult ReEchoRunData::ResolveStartingBuild(const FName CharacterId, const FName WeaponId)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	return ResolveStartingBuildFromSnapshot(Snapshot.Get(), CharacterId, WeaponId);
}

bool ReEchoRunData::TryApplyCardEffectsToBuild(const FReEchoCsvCardRow& Card,
                                               const FReEchoBuildSnapshot& Build,
                                               FReEchoBuildSnapshot& OutBuild)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return false;
	}
	return TryMutateAuthoritativeBuild(
	    *Snapshot,
	    Build,
	    [&](FReEchoBuildSnapshot& Candidate)
	    {
		    return ApplyCardEffects(Card, Candidate);
	    },
	    OutBuild);
}

void UReEchoRunSubsystem::SetPhase(const EReEchoRunPhase NewPhase)
{
	Phase = NewPhase;
	OnPhaseChanged.Broadcast(Phase);
}

void UReEchoRunSubsystem::StartRun(const FName CharacterId, const FName WeaponId)
{
	EncounterIndex = 0;
	TimeShards = 0;
	TraitOfferSeed = MakeNewTraitOfferSeed();
	InventoryItems.Reset();
	OwnedPartIds.Reset();
	bAutomaticAttackMode = true;
	ResetEchoStorage();
	PendingTraitCardIds.Reset();
	PendingEncounterResume = {};
	CurrentBuild = {};
	RunDataSnapshot.Reset();

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoStartRunResolveResult ResolveResult =
	    ReEchoRunData::ResolveStartingBuildFromSnapshot(Snapshot.Get(), CharacterId, WeaponId);
	if (!ResolveResult.bSuccess)
	{
		UE_LOG(LogReEcho, Fatal, TEXT("%s"), *ResolveResult.Error);
	}
	RequireConfiguredBuild(ResolveResult.Build, TEXT("Cannot start run"));
	RunDataSnapshot = Snapshot;
	CurrentBuild = ResolveResult.Build;
	SetPhase(EReEchoRunPhase::Planning);
}

bool UReEchoRunSubsystem::TryEquipParts(const TArray<FName>& PartIds, FString& OutError)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot =
	    RunDataSnapshot.IsValid() ? RunDataSnapshot : FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		OutError = TEXT("CSV snapshot is unavailable");
		return false;
	}
	FReEchoBuildSnapshot Candidate;
	if (!ReEchoWeaponRuntime::TryEquipParts(*Snapshot, CurrentBuild, PartIds, Candidate, OutError))
	{
		return false;
	}
	CurrentBuild = Candidate;
	return true;
}

bool UReEchoRunSubsystem::TrySaveWeaponPartLoadout(const TArray<FName>& PartIds, FString& OutError)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvWeaponRow* Weapon =
	    Snapshot.IsValid() ? Snapshot->FindEnabledWeapon(CurrentBuild.WeaponId) : nullptr;
	if (!Snapshot.IsValid() || !Weapon)
	{
		OutError = TEXT("Cannot save weapon-part loadout: weapon data is unavailable");
		return false;
	}

	TSet<FName> Seen;
	TMap<FName, int32> SlotCounts;
	for (const FName PartId : PartIds)
	{
		if (Seen.Contains(PartId) || !OwnedPartIds.Contains(PartId))
		{
			OutError = FString::Printf(TEXT("Cannot save weapon-part loadout: part '%s' is duplicate or not owned"),
			                           *PartId.ToString());
			return false;
		}
		Seen.Add(PartId);
		const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(PartId);
		if (!Part || !IsPartCompatibleWithWeapon(*Snapshot, *Part, *Weapon))
		{
			OutError =
			    FString::Printf(TEXT("Cannot save weapon-part loadout: part '%s' is incompatible"), *PartId.ToString());
			return false;
		}
		SlotCounts.FindOrAdd(Part->SlotTypeId) += 1;
	}

	for (const TPair<FName, FReEchoCsvSlotProfileRow>& Pair : Snapshot->SlotProfiles)
	{
		const FReEchoCsvSlotProfileRow& Profile = Pair.Value;
		if (Profile.bEnabled && Profile.WeaponTypeId == Weapon->WeaponTypeId && Profile.bRequired &&
		    SlotCounts.FindRef(Profile.SlotTypeId) < Profile.SlotCount)
		{
			OutError = FString::Printf(TEXT("Cannot save weapon-part loadout: required slot '%s' needs %d part(s)"),
			                           *Profile.SlotTypeId.ToString(),
			                           Profile.SlotCount);
			return false;
		}
	}

	return TryEquipParts(PartIds, OutError);
}

FReEchoWeaponPartShopView UReEchoRunSubsystem::GetWeaponPartShopView() const
{
	FReEchoWeaponPartShopView View;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvWeaponRow* Weapon =
	    Snapshot.IsValid() ? Snapshot->FindEnabledWeapon(CurrentBuild.WeaponId) : nullptr;
	if (!Snapshot.IsValid() || !Weapon)
	{
		return View;
	}
	View.WeaponId = Weapon->Id;
	View.WeaponDisplayName = FText::FromString(Weapon->DisplayName);
	View.EquippedParts = CurrentBuild.EquippedParts;

	for (const TPair<FName, FReEchoCsvSlotProfileRow>& Pair : Snapshot->SlotProfiles)
	{
		const FReEchoCsvSlotProfileRow& Profile = Pair.Value;
		if (!Profile.bEnabled || Profile.WeaponTypeId != Weapon->WeaponTypeId)
		{
			continue;
		}
		FReEchoWeaponSlotShopView Slot;
		Slot.SlotTypeId = Profile.SlotTypeId;
		const FReEchoCsvSlotTypeRow* SlotType = Snapshot->SlotTypes.Find(Profile.SlotTypeId);
		Slot.DisplayName = FText::FromString(SlotType ? SlotType->DisplayName : Profile.SlotTypeId.ToString());
		Slot.Capacity = ReEchoWeaponRuntime::GetEffectiveSlotCapacity(
		    *Snapshot, CurrentBuild, Weapon->WeaponTypeId, Profile.SlotTypeId);
		Slot.bRequired = Profile.bRequired;
		View.Slots.Add(Slot);
	}
	View.Slots.Sort(
	    [](const FReEchoWeaponSlotShopView& Left, const FReEchoWeaponSlotShopView& Right)
	    {
		    if (Left.SlotTypeId == TEXT("Core") || Right.SlotTypeId == TEXT("Core"))
		    {
			    return Left.SlotTypeId == TEXT("Core") && Right.SlotTypeId != TEXT("Core");
		    }
		    return Left.SlotTypeId.ToString() < Right.SlotTypeId.ToString();
	    });

	TArray<FReEchoShopOffer> CompatiblePartOffers;
	TArray<FName> PartIds;
	Snapshot->Parts.GetKeys(PartIds);
	PartIds.Sort(
	    [](const FName Left, const FName Right)
	    {
		    return Left.ToString() < Right.ToString();
	    });
	for (const FName PartId : PartIds)
	{
		const FReEchoCsvPartRow& Part = Snapshot->Parts.FindChecked(PartId);
		if (!IsPartCompatibleWithWeapon(*Snapshot, Part, *Weapon))
		{
			continue;
		}
		const FReEchoShopOffer PartOffer = MakeWeaponPartOffer(Part);
		if (Part.bShopEnabled)
		{
			CompatiblePartOffers.Add(PartOffer);
		}
		if (OwnedPartIds.Contains(Part.PartId))
		{
			View.OwnedParts.Add(PartOffer);
		}
	}

	FRandomStream PartRandom(BuildShopOfferSeed(
	    View.WeaponId, EncounterIndex, CurrentBuild.CardState.Runtime.ShopRefreshSequence));
	ShuffleOffers(CompatiblePartOffers, PartRandom);
	for (int32 Index = 0; Index < FMath::Min(ReEchoShopOfferCountPerGroup, CompatiblePartOffers.Num()); ++Index)
	{
		View.Offers.Add(CompatiblePartOffers[Index]);
	}

	if (Snapshot->CardCatalog.IsValid())
	{
		const int32 RefreshSequence = CurrentBuild.CardState.Runtime.ShopRefreshSequence;
		TArray<FReEchoCardDefinition> ShopCards = Snapshot->CardCatalog->GetOfferable(TraitOfferGroup);
		FRandomStream CardRandom(BuildShopOfferSeed(TEXT("SHOP_CARDS"), EncounterIndex, RefreshSequence));
		ShuffleOffers(ShopCards, CardRandom);
		for (const FReEchoCardDefinition& Card : ShopCards)
		{
			const FName OfferId = MakeShopCardOfferId(RefreshSequence, Card.Id);
			const bool bPurchasedOnThisPage = InventoryItems.Contains(OfferId);
			if (!bPurchasedOnThisPage && !ReEchoCardRuntime::CanOffer(*Snapshot->CardCatalog, CurrentBuild.CardState, Card))
			{
				continue;
			}
			View.Offers.Add(MakeBuildCardOffer(Card, RefreshSequence));
			if (View.Offers.FilterByPredicate([](const FReEchoShopOffer& Offer)
			                                     { return Offer.Type == EReEchoShopOfferType::BuildCard; })
			        .Num() >= ReEchoShopOfferCountPerGroup)
			{
				break;
			}
		}

		for (const FName CardId : CurrentBuild.CardState.OwnedCardIds)
		{
			if (const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId))
			{
				FReEchoShopOffer OwnedCard = MakeBuildCardOffer(*Card, RefreshSequence);
				OwnedCard.ItemId = CardId;
				View.OwnedCards.Add(MoveTemp(OwnedCard));
			}
		}
	}
	return View;
}

TSharedPtr<const FReEchoCsvDataSnapshot> UReEchoRunSubsystem::GetRunDataSnapshot() const
{
	return RunDataSnapshot.IsValid() ? RunDataSnapshot : FReEchoCsvDataRegistry::GetSnapshot();
}

int32 UReEchoRunSubsystem::GetTotalEncounterCount() const
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	int32 EnabledCount = 0;
	if (Snapshot.IsValid())
	{
		for (const FName EncounterId : Snapshot->EncounterOrder)
		{
			const FReEchoCsvEncounterRow* Encounter = Snapshot->FindEncounter(EncounterId);
			EnabledCount += Encounter && Encounter->bEnabled ? 1 : 0;
		}
	}
	return EnabledCount > 0 ? EnabledCount : GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount();
}

void UReEchoRunSubsystem::BeginEncounter()
{
	++EncounterIndex;
	CurrentBuild.CardState = ReEchoCardRuntime::BeginEncounter(CurrentBuild.CardState, EncounterIndex);
	bPendingCardEchoRemoval = false;
	SetPhase(EReEchoRunPhase::Encounter);
}

void UReEchoRunSubsystem::CompleteEncounter(const FReEchoRecording& Recording,
                                            const bool bPlayerSurvived,
                                            const bool bBossKilled)
{
	if (!bPlayerSurvived)
	{
		// A failed encounter never becomes storage eligible and never touches the rolling latest echo.
		SetPhase(EReEchoRunPhase::Failed);
		return;
	}
	StagePendingRecording(Recording);
	const FReEchoCardRuleSnapshot CardRules = GetCardRules();
	if (!CardRules.bDisableEnemyShardDrops)
	{
		const float RewardMultiplier =
		    CurrentBuild.CardState.Runtime.BonusShardDropEncounterIndex == EncounterIndex ? 1.5f : 1.0f;
		TimeShards += FMath::RoundToInt(15.0f * RewardMultiplier);
	}
	if (CurrentBuild.CardState.Runtime.BonusShardDropEncounterIndex == EncounterIndex)
	{
		CurrentBuild.CardState.Runtime.BonusShardDropEncounterIndex = INDEX_NONE;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> CardSnapshot = GetRunDataSnapshot();
	FReEchoBuildSnapshot CardEndBuild;
	if (CardSnapshot.IsValid() && CardSnapshot->CardCatalog.IsValid() &&
	    TryMutateAuthoritativeBuild(
	        *CardSnapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardEventResult Event = ReEchoCardRuntime::EndEncounter(
		            *CardSnapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats, EncounterIndex, 0);
		        BaseBuild.CardState = Event.CardState;
		        BaseBuild.Stats = Event.Stats;
		        return true;
	        },
	        CardEndBuild))
	{
		CurrentBuild = CardEndBuild;
	}
	if (CurrentCharacterHasPassive(CurrentBuild, TEXT("Character.PoetReactionGrowth")))
	{
		const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
		const FReEchoCsvCharacterRow* Character =
		    Snapshot.IsValid() ? Snapshot->FindCharacter(CurrentBuild.CharacterId) : nullptr;
		FReEchoBuildSnapshot Candidate;
		if (Snapshot.IsValid() && TryMutateAuthoritativeBuild(
		                              *Snapshot,
		                              CurrentBuild,
		                              [&](FReEchoBuildSnapshot& BaseBuild)
		                              {
			                              BaseBuild.Stats.ReactionEfficiency +=
			                                  Character ? Character->PassiveValue : 0.05f;
			                              return true;
		                              },
		                              Candidate))
		{
			CurrentBuild = Candidate;
		}
	}
	if (EncounterIndex >= GetTotalEncounterCount())
	{
		SetPhase(bBossKilled ? EReEchoRunPhase::Summary : EReEchoRunPhase::Failed);
		return;
	}
	SetPhase(CurrentCharacterHasPassive(CurrentBuild, TEXT("Character.BraveForge")) ? EReEchoRunPhase::ForgeChoice
	                                                                                : EReEchoRunPhase::CardChoice);
}

TArray<FReEchoTraitCardOffer> UReEchoRunSubsystem::GenerateTraitCardOffers(const int32 RequestedCount)
{
	PendingTraitCardIds.Reset();
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const TArray<FReEchoCardDefinition> Catalog =
	    Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()
	        ? ReEchoCardRuntime::BuildOfferPool(*Snapshot->CardCatalog, CurrentBuild.CardState, TraitOfferGroup)
	        : TArray<FReEchoCardDefinition>();
	const int32 OfferCount = FMath::Clamp(RequestedCount, 0, Catalog.Num());
	if (OfferCount == 0 || Phase != EReEchoRunPhase::CardChoice)
	{
		return {};
	}

	FRandomStream Random(BuildTraitOfferSeed(TraitOfferSeed, EncounterIndex, CurrentBuild.CardState.OwnedCardIds));
	TArray<FReEchoTraitCardOffer> Result;
	int32 StackLevel = 0;
	while (Result.Num() < OfferCount)
	{
		TArray<FReEchoCardDefinition> StackBucket;
		for (const FReEchoCardDefinition& Card : Catalog)
		{
			if (ReEchoCardRuntime::CountOwned(CurrentBuild.CardState, Card.Id) == StackLevel)
			{
				StackBucket.Add(Card);
			}
		}

		ShuffleOffers(StackBucket, Random);
		for (const FReEchoCardDefinition& Card : StackBucket)
		{
			if (Result.Num() >= OfferCount)
			{
				break;
			}
			Result.Add(MakeTraitOffer(Card));
		}
		++StackLevel;
	}

	for (const FReEchoTraitCardOffer& Offer : Result)
	{
		PendingTraitCardIds.Add(Offer.CardId);
	}
	return Result;
}

bool UReEchoRunSubsystem::ApplyTraitCard(const FName CardId)
{
	if (Phase != EReEchoRunPhase::CardChoice || !PendingTraitCardIds.Contains(CardId))
	{
		return false;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid() || !Snapshot->CardCatalog->Find(CardId))
	{
		return false;
	}

	const FName SageBonusChoiceFlag = TEXT("SageBonusChoice");
	const FName NormalTraitSelectionsFlag = TEXT("NormalTraitSelections");
	const bool bSageBonusChoice = CurrentBuild.RuleFlags.Contains(SageBonusChoiceFlag);
	bool bSageBonus = false;
	int32 PendingTimeShards = TimeShards;
	FReEchoBuildSnapshot PendingBuild;
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        FReEchoCardGrantInput Input;
		        Input.Stats = BaseBuild.Stats;
		        Input.CardState = BaseBuild.CardState;
		        Input.TimeShards = PendingTimeShards;
		        Input.EncounterIndex = EncounterIndex;
		        Input.RandomSeed =
		            BuildTraitOfferSeed(TraitOfferSeed, EncounterIndex, BaseBuild.CardState.OwnedCardIds);
		        const FReEchoCardGrantResult Grant =
		            ReEchoCardRuntime::TryGrantCard(*Snapshot->CardCatalog, CardId, Input);
		        if (!Grant.bSucceeded)
		        {
			        return false;
		        }
		        BaseBuild.Stats = Grant.Stats;
		        BaseBuild.CardState = Grant.CardState;
		        PendingTimeShards = Grant.TimeShards;
		        ReEchoCharacterPromotion::TryPromote(BaseBuild);
		        if (bSageBonusChoice)
		        {
			        BaseBuild.RuleFlags.Remove(SageBonusChoiceFlag);
			        return true;
		        }
		        const int32 NormalTraitSelections =
		            FCString::Atoi(*BaseBuild.RuleFlags.FindRef(NormalTraitSelectionsFlag)) + 1;
		        BaseBuild.RuleFlags.Add(NormalTraitSelectionsFlag, FString::FromInt(NormalTraitSelections));
		        bSageBonus =
		            ReEchoCharacterPromotion::IsRole(BaseBuild, TEXT("Sage")) && NormalTraitSelections % 4 == 0;
		        if (bSageBonus)
		        {
			        BaseBuild.RuleFlags.Add(SageBonusChoiceFlag, TEXT("1"));
		        }
		        return true;
	        },
	        PendingBuild))
	{
		return false;
	}
	CurrentBuild = PendingBuild;
	TimeShards = PendingTimeShards;
	PendingTraitCardIds.Reset();
	SetPhase(bSageBonus ? EReEchoRunPhase::CardChoice : EReEchoRunPhase::Planning);
	return true;
}

bool UReEchoRunSubsystem::DebugGrantCard(const FName CardId)
{
	if (CardId.IsNone())
	{
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid() || !Snapshot->CardCatalog->Find(CardId))
	{
		return false;
	}

	FReEchoBuildSnapshot PendingBuild;
	if (!TryMutateAuthoritativeBuild(
			*Snapshot,
			CurrentBuild,
			[&](FReEchoBuildSnapshot& BaseBuild)
			{
				FReEchoCardGrantInput Input;
				Input.Stats = BaseBuild.Stats;
				Input.CardState = BaseBuild.CardState;
				Input.TimeShards = TimeShards;
				Input.EncounterIndex = EncounterIndex;
				const FReEchoCardGrantResult Grant =
					ReEchoCardRuntime::TryGrantCard(*Snapshot->CardCatalog, CardId, Input);
				if (!Grant.bSucceeded)
				{
					return false;
				}
				BaseBuild.Stats = Grant.Stats;
				BaseBuild.CardState = Grant.CardState;
				ReEchoCharacterPromotion::TryPromote(BaseBuild);
				return true;
			},
			PendingBuild))
	{
		return false;
	}
	CurrentBuild = PendingBuild;
	return true;
}

TArray<FReEchoTraitCardOffer> UReEchoRunSubsystem::GenerateForgeOffers()
{
	PendingTraitCardIds.Reset();
	if (Phase != EReEchoRunPhase::ForgeChoice)
	{
		return {};
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	TArray<FReEchoTraitCardOffer> Offers;
	const TArray<FReEchoCardDefinition> ForgeCards = Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()
	                                                     ? Snapshot->CardCatalog->GetOfferable(ForgeOfferGroup)
	                                                     : TArray<FReEchoCardDefinition>();
	for (const FReEchoCardDefinition& Card : ForgeCards)
	{
		Offers.Add(MakeTraitOffer(Card));
	}
	for (const FReEchoTraitCardOffer& Offer : Offers)
	{
		PendingTraitCardIds.Add(Offer.CardId);
	}
	return Offers;
}

bool UReEchoRunSubsystem::ApplyForgeChoice(const FName ForgeId)
{
	if (Phase != EReEchoRunPhase::ForgeChoice || !PendingTraitCardIds.Contains(ForgeId))
	{
		return false;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCardDefinition* Card =
	    Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() ? Snapshot->CardCatalog->Find(ForgeId) : nullptr;
	FReEchoBuildSnapshot PendingBuild;
	if (!Card || !Card->bEnabled || Card->OfferGroup != ForgeOfferGroup ||
	    !TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        FReEchoCardGrantInput Input;
		        Input.Stats = BaseBuild.Stats;
		        Input.CardState = BaseBuild.CardState;
		        Input.TimeShards = TimeShards;
		        Input.EncounterIndex = EncounterIndex;
		        Input.RandomSeed =
		            BuildTraitOfferSeed(TraitOfferSeed, EncounterIndex, BaseBuild.CardState.OwnedCardIds);
		        Input.bRecordOwnership = false;
		        const FReEchoCardGrantResult Grant =
		            ReEchoCardRuntime::TryGrantCard(*Snapshot->CardCatalog, ForgeId, Input);
		        if (!Grant.bSucceeded)
		        {
			        return false;
		        }
		        BaseBuild.Stats = Grant.Stats;
		        BaseBuild.CardState = Grant.CardState;
		        BaseBuild.Stats.HpMax = FMath::Max(1.0f, BaseBuild.Stats.HpMax);
		        return true;
	        },
	        PendingBuild))
	{
		return false;
	}

	CurrentBuild = PendingBuild;
	PendingTraitCardIds.Reset();
	SetPhase(EReEchoRunPhase::CardChoice);
	return true;
}

FReEchoCardRuleSnapshot UReEchoRunSubsystem::GetCardRules() const
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	return Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()
	           ? ReEchoCardRuntime::CompileRules(*Snapshot->CardCatalog, CurrentBuild.CardState)
	           : FReEchoCardRuleSnapshot{};
}

FReEchoCardEncounterTickResult UReEchoRunSubsystem::AdvanceCardEncounter(const float EncounterTimeSeconds)
{
	FReEchoCardEncounterTickResult Result;
	Result.CardState = CurrentBuild.CardState;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (Snapshot.IsValid() && Snapshot->CardCatalog.IsValid())
	{
		Result =
		    ReEchoCardRuntime::AdvanceEncounter(*Snapshot->CardCatalog, CurrentBuild.CardState, EncounterTimeSeconds);
		CurrentBuild.CardState = Result.CardState;
	}
	return Result;
}

void UReEchoRunSubsystem::ModifyCardOutgoingHit(FReEchoHitIntent& Intent,
                                                const FReEchoStatBlock& SourceStats,
                                                const float EchoDistanceCm,
                                                const bool bTargetHasElement)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return;
	}
	FReEchoCardOutgoingHitInput Input;
	Input.RawDamage = Intent.RawDamage;
	Input.CriticalRate = SourceStats.CriticalRate;
	Input.CriticalEffect = SourceStats.CriticalEffect;
	Input.DistanceCm = EchoDistanceCm;
	Input.bCritical = Intent.bCritical;
	Input.bTargetHasElement = bTargetHasElement;
	Input.Element = Intent.Element;
	Input.TimeShards = TimeShards;
	Input.RandomSeed = HashCombine(GetTypeHash(EncounterIndex), GetTypeHash(Intent.Attack.Sequence));
	const FReEchoCardOutgoingHitResult Result =
	    ReEchoCardRuntime::ModifyOutgoingHit(*Snapshot->CardCatalog, CurrentBuild.CardState, Input);
	CurrentBuild.CardState = Result.CardState;
	TimeShards = Result.TimeShards;
	Intent.RawDamage = Result.RawDamage;
	Intent.bCritical = Result.bCritical;
	Intent.Element = Result.Element;
}

float UReEchoRunSubsystem::ModifyCardIncomingHit(const float RawDamage)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return RawDamage;
	}
	const FReEchoCardIncomingHitResult Result =
	    ReEchoCardRuntime::ModifyIncomingHit(*Snapshot->CardCatalog, CurrentBuild.CardState, RawDamage, TimeShards);
	CurrentBuild.CardState = Result.CardState;
	TimeShards = Result.TimeShards;
	bPendingCardEchoRemoval |= Result.bRemoveAllEchoes;
	return Result.RawDamage;
}

float UReEchoRunSubsystem::NotifyCardReaction(const FName ReactionId, const bool bTriggeredByPlayer)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return 0.0f;
	}
	float Healing = 0.0f;
	FReEchoBuildSnapshot PendingBuild;
	if (TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardEventResult Event = ReEchoCardRuntime::OnReaction(
		            *Snapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats, ReactionId, bTriggeredByPlayer);
		        BaseBuild.CardState = Event.CardState;
		        BaseBuild.Stats = Event.Stats;
		        Healing = Event.Healing;
		        return true;
	        },
	        PendingBuild))
	{
		CurrentBuild = PendingBuild;
	}
	return Healing;
}

void UReEchoRunSubsystem::NotifyCardKill(const bool bKilledByEcho)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	FReEchoBuildSnapshot PendingBuild;
	if (Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() &&
	    TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardEventResult Event = ReEchoCardRuntime::OnKillResolved(
		            *Snapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats, bKilledByEcho);
		        BaseBuild.CardState = Event.CardState;
		        BaseBuild.Stats = Event.Stats;
		        return true;
	        },
	        PendingBuild))
	{
		CurrentBuild = PendingBuild;
	}
}

void UReEchoRunSubsystem::NotifyCardEchoDefeated()
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	FReEchoBuildSnapshot PendingBuild;
	if (Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() &&
	    TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardEventResult Event =
		            ReEchoCardRuntime::OnEchoKilled(*Snapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats);
		        BaseBuild.CardState = Event.CardState;
		        BaseBuild.Stats = Event.Stats;
		        return true;
	        },
	        PendingBuild))
	{
		CurrentBuild = PendingBuild;
	}
}

bool UReEchoRunSubsystem::ConsumeCardEchoRemovalRequest()
{
	const bool bResult = bPendingCardEchoRemoval;
	bPendingCardEchoRemoval = false;
	return bResult;
}

int32 UReEchoRunSubsystem::GetDiscountedShopPrice(const int32 BasePrice) const
{
	return FMath::Max(0, FMath::CeilToInt(BasePrice * (1.0f - GetCardRules().ShopDiscount)));
}

bool UReEchoRunSubsystem::TryConsumeShopRefresh(const int32 PaidRefreshPrice)
{
	const FReEchoCardRuleSnapshot Rules = GetCardRules();
	if (Rules.bDisableShopRefresh)
	{
		return false;
	}
	if (CurrentBuild.CardState.Runtime.FreeShopRefreshes > 0)
	{
		--CurrentBuild.CardState.Runtime.FreeShopRefreshes;
		++CurrentBuild.CardState.Runtime.ShopRefreshSequence;
		return true;
	}
	if (PaidRefreshPrice <= 0)
	{
		return false;
	}
	const int32 Price = PaidRefreshPrice;
	if (TimeShards < Price)
	{
		return false;
	}
	TimeShards -= Price;
	++CurrentBuild.CardState.Runtime.ShopRefreshSequence;
	return true;
}

bool UReEchoRunSubsystem::CanPurchaseExtraShopCard() const
{
	return !GetCardRules().bDisableExtraCardPurchase;
}

bool UReEchoRunSubsystem::SetCardAnchorRecording(const FGuid RecordingId)
{
	if (!ReEchoCardRuntime::HasCard(CurrentBuild.CardState, TEXT("G_3_02")) ||
	    FindStoredEchoIndex(RecordingId) == INDEX_NONE)
	{
		return false;
	}
	CurrentBuild.CardState.Runtime.bHasAnchorRecording = true;
	CurrentBuild.CardState.Runtime.AnchorRecordingId = RecordingId;
	return true;
}

void UReEchoRunSubsystem::ClearCardAnchorRecording()
{
	CurrentBuild.CardState.Runtime.bHasAnchorRecording = false;
	CurrentBuild.CardState.Runtime.AnchorRecordingId.Invalidate();
}

bool UReEchoRunSubsystem::PurchaseShopItem(const FName ItemId)
{
	const FReEchoWeaponPartShopView ShopView = GetWeaponPartShopView();
	const FReEchoShopOffer* Offer = ShopView.Offers.FindByPredicate(
	    [&](const FReEchoShopOffer& Candidate)
	    {
		    return Candidate.ItemId == ItemId;
	    });
	FReEchoShopOffer LegacyOffer;
	if (!Offer)
	{
		if (const FReEchoShopOffer* Legacy = GetReEchoShopCatalog().FindByPredicate(
		        [&](const FReEchoShopOffer& Candidate) { return Candidate.ItemId == ItemId; }))
		{
			LegacyOffer = *Legacy;
			LegacyOffer.ContentId = LegacyOffer.ItemId;
			Offer = &LegacyOffer;
		}
	}
	if (!Offer || ((Offer->Type == EReEchoShopOfferType::RunItem ||
	                Offer->Type == EReEchoShopOfferType::BuildCard) &&
	               InventoryItems.Contains(ItemId)) ||
	    (Offer->Type == EReEchoShopOfferType::WeaponPart && OwnedPartIds.Contains(Offer->ContentId)))
	{
		return false;
	}
	if (Offer->Type == EReEchoShopOfferType::BuildCard && !CanPurchaseExtraShopCard())
	{
		return false;
	}
	const int32 EffectivePrice = GetDiscountedShopPrice(Offer->Price);
	if (TimeShards < EffectivePrice)
	{
		return false;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoBuildSnapshot OriginalBuild = CurrentBuild;
	int32 PendingTimeShards = TimeShards;
	FReEchoBuildSnapshot PendingBuild;
	if (!Snapshot.IsValid() ||
	    !TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        if (Offer->Type == EReEchoShopOfferType::BuildCard)
		        {
			        if (!Snapshot->CardCatalog.IsValid())
			        {
				        return false;
			        }
			        FReEchoCardGrantInput Input;
			        Input.Stats = BaseBuild.Stats;
			        Input.CardState = BaseBuild.CardState;
			        Input.TimeShards = PendingTimeShards;
			        Input.EncounterIndex = EncounterIndex;
			        Input.RandomSeed = BuildShopOfferSeed(
			            Offer->ContentId, EncounterIndex, BaseBuild.CardState.Runtime.ShopRefreshSequence);
			        const FReEchoCardGrantResult Grant = ReEchoCardRuntime::TryGrantCard(
			            *Snapshot->CardCatalog, Offer->ContentId, Input);
			        if (!Grant.bSucceeded)
			        {
				        return false;
			        }
			        BaseBuild.Stats = Grant.Stats;
			        BaseBuild.CardState = Grant.CardState;
			        PendingTimeShards = Grant.TimeShards;
		        }
		        if (Offer->Type == EReEchoShopOfferType::RunItem && ItemId == TEXT("SHOP_RUSTED_SCISSORS"))
		        {
			        BaseBuild.Stats.PhysicalAttack += 2.0f;
		        }
		        else if (Offer->Type == EReEchoShopOfferType::RunItem && ItemId == TEXT("SHOP_DREAM_FRUIT"))
		        {
			        BaseBuild.Stats.HpMax += 10.0f;
		        }
		        else if (Offer->Type == EReEchoShopOfferType::RunItem && ItemId == TEXT("SHOP_BLACK_FEATHER"))
		        {
			        BaseBuild.Stats.MovementSpeed += 0.1f;
		        }
		        else if (Offer->Type == EReEchoShopOfferType::RunItem && ItemId == TEXT("SHOP_OLD_COIN"))
		        {
			        BaseBuild.Stats.EchoEfficiency += 0.1f;
		        }
		        else if (Offer->Type == EReEchoShopOfferType::RunItem && ItemId == TEXT("SHOP_REPLAY_UNLOCK"))
		        {
			        // 不修改 build 属性；只解锁指定回放槽位上限（见下方统一处理）。
		        }
		        if (Snapshot->CardCatalog.IsValid())
		        {
			        const FReEchoCardEventResult Event =
			            ReEchoCardRuntime::OnPurchase(*Snapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats);
			        BaseBuild.CardState = Event.CardState;
			        BaseBuild.Stats = Event.Stats;
		        }
		        return true;
	        },
	        PendingBuild))
	{
		return false;
	}
	PendingTimeShards -= EffectivePrice;
	TimeShards = PendingTimeShards;
	if (Offer->Type == EReEchoShopOfferType::WeaponPart)
	{
		OwnedPartIds.Add(Offer->ContentId);
	}
	else
	{
		InventoryItems.Add(ItemId);
	}
	CurrentBuild = PendingBuild;

	if (ItemId == TEXT("SHOP_REPLAY_UNLOCK"))
	{
		// 一次性解锁：把指定回放槽位上限拉满（3），不修改 build 属性。
		// 通用查重/余额检查（上方）已保证原子拒绝重复购买与碎片不足。
		const EReEchoEchoStorageResult LimitResult = SetSpecificReplayLimit(ReEchoEchoStorage::MaxSpecificReplayLimit);
		if (LimitResult != EReEchoEchoStorageResult::Success)
		{
			TimeShards += EffectivePrice; // 设定失败则回滚扣费
			InventoryItems.Remove(ItemId);
			CurrentBuild = OriginalBuild;
			return false;
		}
	}
	return true;
}

void UReEchoRunSubsystem::ResetEchoStorage()
{
	bHasPendingRecording = false;
	PendingRecording = {};
	bHasLatestCompletedRecording = false;
	LatestCompletedRecording = {};
	bHasPreviousCompletedRecording = false;
	PreviousCompletedRecording = {};
	StoredEchoes.Reset();
	SelectedReplayIds.Reset();
	StorageCapacity = ReEchoEchoStorage::DefaultStorageCapacity;
	SpecificReplayLimit = ReEchoEchoStorage::SpecificReplayUnavailable;
}

int32 UReEchoRunSubsystem::FindStoredEchoIndex(const FGuid& RecordingId) const
{
	if (!RecordingId.IsValid())
	{
		return INDEX_NONE;
	}
	return StoredEchoes.IndexOfByPredicate(
	    [&RecordingId](const FReEchoRecording& Stored)
	    {
		    return Stored.Id == RecordingId;
	    });
}

void UReEchoRunSubsystem::NormalizeSelectedReplayIds()
{
	TArray<FGuid> Normalized;
	Normalized.Reserve(SelectedReplayIds.Num());
	for (const FGuid& SelectedId : SelectedReplayIds)
	{
		// Selections only ever reference stored echoes, so a removed or replaced echo drops out here.
		if (FindStoredEchoIndex(SelectedId) == INDEX_NONE || Normalized.Contains(SelectedId))
		{
			continue;
		}
		Normalized.Add(SelectedId);
	}
	const int32 AllowedCount = FMath::Clamp(SpecificReplayLimit, 0, ReEchoEchoStorage::MaxSpecificReplayLimit);
	if (Normalized.Num() > AllowedCount)
	{
		Normalized.SetNum(AllowedCount);
	}
	SelectedReplayIds = MoveTemp(Normalized);
}

EReEchoEchoStorageResult UReEchoRunSubsystem::StagePendingRecording(const FReEchoRecording& Recording)
{
	if (!Recording.Id.IsValid())
	{
		return EReEchoEchoStorageResult::InvalidRecordingId;
	}
	if (FindStoredEchoIndex(Recording.Id) != INDEX_NONE)
	{
		return EReEchoEchoStorageResult::DuplicateRecordingId;
	}
	PendingRecording = Recording;
	bHasPendingRecording = true;
	if (bHasLatestCompletedRecording && LatestCompletedRecording.Id != Recording.Id)
	{
		PreviousCompletedRecording = LatestCompletedRecording;
		bHasPreviousCompletedRecording = true;
	}
	LatestCompletedRecording = Recording;
	bHasLatestCompletedRecording = true;
	return EReEchoEchoStorageResult::Success;
}

EReEchoEchoStorageResult UReEchoRunSubsystem::SkipPendingRecordingStorage()
{
	if (!bHasPendingRecording)
	{
		return EReEchoEchoStorageResult::NoPendingRecording;
	}
	// Skipping only drops permanent storage eligibility; the rolling latest echo stays intact.
	bHasPendingRecording = false;
	PendingRecording = {};
	return EReEchoEchoStorageResult::Success;
}

EReEchoEchoStorageResult UReEchoRunSubsystem::StorePendingRecording()
{
	if (!bHasPendingRecording)
	{
		return EReEchoEchoStorageResult::NoPendingRecording;
	}
	if (!PendingRecording.Id.IsValid())
	{
		return EReEchoEchoStorageResult::InvalidRecordingId;
	}
	if (FindStoredEchoIndex(PendingRecording.Id) != INDEX_NONE)
	{
		return EReEchoEchoStorageResult::DuplicateRecordingId;
	}
	if (StoredEchoes.Num() >= FMath::Max(0, StorageCapacity))
	{
		// Full storage never auto-evicts; the caller must name an explicit replacement target.
		return EReEchoEchoStorageResult::StorageFull;
	}
	StoredEchoes.Add(PendingRecording);
	bHasPendingRecording = false;
	PendingRecording = {};
	return EReEchoEchoStorageResult::Success;
}

EReEchoEchoStorageResult UReEchoRunSubsystem::StorePendingRecordingReplacing(const FGuid ReplacedRecordingId)
{
	if (!bHasPendingRecording)
	{
		return EReEchoEchoStorageResult::NoPendingRecording;
	}
	if (!PendingRecording.Id.IsValid())
	{
		return EReEchoEchoStorageResult::InvalidRecordingId;
	}
	const int32 ReplacedIndex = FindStoredEchoIndex(ReplacedRecordingId);
	if (ReplacedIndex == INDEX_NONE)
	{
		return EReEchoEchoStorageResult::InvalidReplacementTarget;
	}
	if (FindStoredEchoIndex(PendingRecording.Id) != INDEX_NONE)
	{
		return EReEchoEchoStorageResult::DuplicateRecordingId;
	}
	StoredEchoes[ReplacedIndex] = PendingRecording;
	if (CurrentBuild.CardState.Runtime.bHasAnchorRecording &&
	    CurrentBuild.CardState.Runtime.AnchorRecordingId == ReplacedRecordingId)
	{
		ClearCardAnchorRecording();
	}
	bHasPendingRecording = false;
	PendingRecording = {};
	NormalizeSelectedReplayIds();
	return EReEchoEchoStorageResult::Success;
}

EReEchoEchoStorageResult UReEchoRunSubsystem::SetSelectedReplayIds(const TArray<FGuid>& RequestedIds)
{
	const int32 AllowedCount = FMath::Clamp(SpecificReplayLimit, 0, ReEchoEchoStorage::MaxSpecificReplayLimit);
	if (RequestedIds.Num() > AllowedCount)
	{
		return EReEchoEchoStorageResult::ReplayLimitExceeded;
	}
	TArray<FGuid> Validated;
	Validated.Reserve(RequestedIds.Num());
	for (const FGuid& RequestedId : RequestedIds)
	{
		if (FindStoredEchoIndex(RequestedId) == INDEX_NONE)
		{
			return EReEchoEchoStorageResult::InvalidRecordingId;
		}
		if (Validated.Contains(RequestedId))
		{
			return EReEchoEchoStorageResult::DuplicateRecordingId;
		}
		Validated.Add(RequestedId);
	}
	SelectedReplayIds = MoveTemp(Validated);
	return EReEchoEchoStorageResult::Success;
}

EReEchoEchoStorageResult UReEchoRunSubsystem::SetStorageCapacity(const int32 NewCapacity)
{
	if (NewCapacity < 0 || NewCapacity > ReEchoEchoStorage::MaxStorageCapacity || NewCapacity < StoredEchoes.Num())
	{
		// Shrinking below the current occupancy would silently delete player-chosen echoes.
		return EReEchoEchoStorageResult::InvalidStorageCapacity;
	}
	StorageCapacity = NewCapacity;
	return EReEchoEchoStorageResult::Success;
}

EReEchoEchoStorageResult UReEchoRunSubsystem::SetSpecificReplayLimit(const int32 NewLimit)
{
	if (NewLimit < 0 || NewLimit > ReEchoEchoStorage::MaxSpecificReplayLimit)
	{
		return EReEchoEchoStorageResult::InvalidReplayLimit;
	}
	SpecificReplayLimit = NewLimit;
	NormalizeSelectedReplayIds();
	return EReEchoEchoStorageResult::Success;
}

FReEchoEchoStorageSummary UReEchoRunSubsystem::GetEchoStorageSummary() const
{
	FReEchoEchoStorageSummary Summary;
	Summary.StorageCapacity = StorageCapacity;
	Summary.SpecificReplayLimit = SpecificReplayLimit;
	Summary.SelectedReplayIds = SelectedReplayIds;
	Summary.StoredEchoes.Reserve(StoredEchoes.Num());
	for (const FReEchoRecording& Stored : StoredEchoes)
	{
		Summary.StoredEchoes.Add(MakeStoredEchoSummary(Stored, SelectedReplayIds.Contains(Stored.Id)));
	}
	Summary.bHasPendingRecording = bHasPendingRecording;
	if (bHasPendingRecording)
	{
		Summary.PendingRecording = MakeStoredEchoSummary(PendingRecording, false);
	}
	Summary.bHasLatestCompletedRecording = bHasLatestCompletedRecording;
	if (bHasLatestCompletedRecording)
	{
		Summary.LatestCompletedRecording = MakeStoredEchoSummary(LatestCompletedRecording, false);
	}
	return Summary;
}

bool UReEchoRunSubsystem::TryGetStoredEcho(const FGuid RecordingId, FReEchoRecording& OutRecording) const
{
	const int32 StoredIndex = FindStoredEchoIndex(RecordingId);
	if (StoredIndex == INDEX_NONE)
	{
		return false;
	}
	OutRecording = StoredEchoes[StoredIndex];
	return true;
}

bool UReEchoRunSubsystem::TryGetPendingRecording(FReEchoRecording& OutRecording) const
{
	if (!bHasPendingRecording)
	{
		return false;
	}
	OutRecording = PendingRecording;
	return true;
}

bool UReEchoRunSubsystem::TryGetLatestCompletedRecording(FReEchoRecording& OutRecording) const
{
	if (!bHasLatestCompletedRecording)
	{
		return false;
	}
	OutRecording = LatestCompletedRecording;
	return true;
}

TArray<FReEchoRecording> UReEchoRunSubsystem::ResolveReplayRecordings(const int32 RequestedCount) const
{
	TArray<FReEchoRecording> Result;
	const FReEchoCardRuleSnapshot Rules = GetCardRules();
	if (Rules.bEchoesDisabled)
	{
		return Result;
	}
	const int32 Count = FMath::Clamp(RequestedCount, 0, ReEchoEchoStorage::MaxStorageCapacity);
	if (Count == 0)
	{
		return Result;
	}
	if (CurrentBuild.CardState.Runtime.bHasAnchorRecording)
	{
		FReEchoRecording Anchor;
		if (TryGetStoredEcho(CurrentBuild.CardState.Runtime.AnchorRecordingId, Anchor))
		{
			Result.Add(MoveTemp(Anchor));
		}
		return Result;
	}

	const int32 AllowedCount = FMath::Clamp(SpecificReplayLimit, 0, ReEchoEchoStorage::MaxSpecificReplayLimit);

	// Once the specific replay ability is unlocked, explicit selections take priority. If the
	// player has not selected anything yet, keep the Plan32 automatic fallback to the rolling
	// previous encounter so the transition never silently loses all replay.
	if (AllowedCount > 0)
	{
		if (SelectedReplayIds.Num() == 0)
		{
			const int32 DefaultCount = FMath::Min(Count, Rules.MaximumEchoes);
			if (DefaultCount > 0 && bHasLatestCompletedRecording)
			{
				Result.Add(LatestCompletedRecording);
			}
			if (Result.Num() < DefaultCount && bHasPreviousCompletedRecording &&
			    (!bHasLatestCompletedRecording || PreviousCompletedRecording.Id != LatestCompletedRecording.Id))
			{
				Result.Add(PreviousCompletedRecording);
			}
			return Result;
		}

		const int32 ResolveCount = FMath::Min3(Count, AllowedCount, SelectedReplayIds.Num());
		for (const FGuid& SelectedId : SelectedReplayIds)
		{
			if (Result.Num() >= ResolveCount)
			{
				break;
			}
			FReEchoRecording Selected;
			if (TryGetStoredEcho(SelectedId, Selected))
			{
				Result.Add(MoveTemp(Selected));
			}
		}
		return Result;
	}

	// Specific replay not unlocked: the next encounter automatically replays the rolling
	// previous-encounter echo. Stored echoes and any residual selection cannot override this.
	const int32 DefaultCount = FMath::Min(Count, Rules.MaximumEchoes);
	if (DefaultCount > 0 && bHasLatestCompletedRecording)
	{
		Result.Add(LatestCompletedRecording);
	}
	if (Result.Num() < DefaultCount && bHasPreviousCompletedRecording &&
	    (!bHasLatestCompletedRecording || PreviousCompletedRecording.Id != LatestCompletedRecording.Id))
	{
		Result.Add(PreviousCompletedRecording);
	}
	return Result;
}

TArray<FReEchoRecording> UReEchoRunSubsystem::GetEchoRecordings(const int32 RequestedCount) const
{
	// Legacy compatibility facade only; it owns no state and forwards to the new resolution rules.
	return ResolveReplayRecordings(RequestedCount);
}

bool UReEchoRunSubsystem::HasSavedRun() const
{
	const UReEchoRunSaveGame* SaveGame =
	    Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromSlot(RunSaveSlot, RunSaveUserIndex));
	if (!SaveGame || !IsValidResumableSave(*SaveGame))
	{
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	return Snapshot.IsValid() &&
	       ReEchoWeaponRuntime::GetBuildConfigurationError(*Snapshot, SaveGame->CurrentBuild).IsEmpty();
}

bool UReEchoRunSubsystem::SaveRun(const FReEchoEncounterRuntimeState* EncounterRuntimeState) const
{
	UReEchoRunSaveGame* SaveGame = CreateSaveSnapshot(EncounterRuntimeState);
	return SaveGame && UGameplayStatics::SaveGameToSlot(SaveGame, RunSaveSlot, RunSaveUserIndex);
}

bool UReEchoRunSubsystem::LoadSavedRun()
{
	const UReEchoRunSaveGame* SaveGame =
	    Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromSlot(RunSaveSlot, RunSaveUserIndex));
	return SaveGame && RestoreSaveSnapshot(*SaveGame);
}

void UReEchoRunSubsystem::DeleteSavedRun() const
{
	if (UGameplayStatics::DoesSaveGameExist(RunSaveSlot, RunSaveUserIndex))
	{
		UGameplayStatics::DeleteGameInSlot(RunSaveSlot, RunSaveUserIndex);
	}
}

bool UReEchoRunSubsystem::HasPendingEncounterResume() const
{
	return PendingEncounterResume.bValid;
}

FReEchoEncounterRuntimeState UReEchoRunSubsystem::ConsumePendingEncounterResume()
{
	FReEchoEncounterRuntimeState Result = PendingEncounterResume;
	PendingEncounterResume = {};
	return Result;
}

UReEchoRunSaveGame*
UReEchoRunSubsystem::CreateSaveSnapshot(const FReEchoEncounterRuntimeState* EncounterRuntimeState) const
{
	UReEchoRunSaveGame* SaveGame = NewObject<UReEchoRunSaveGame>(GetTransientPackage());
	SaveGame->EncounterIndex = EncounterIndex;
	SaveGame->SavedPhase = Phase;
	if (Phase == EReEchoRunPhase::Encounter && EncounterRuntimeState && EncounterRuntimeState->bValid)
	{
		SaveGame->EncounterRuntimeState = *EncounterRuntimeState;
	}
	else if (Phase == EReEchoRunPhase::Encounter)
	{
		SaveGame->EncounterIndex = FMath::Max(0, EncounterIndex - 1);
		SaveGame->SavedPhase = EReEchoRunPhase::Planning;
	}
	SaveGame->TimeShards = TimeShards;
	SaveGame->TraitOfferSeed = TraitOfferSeed;
	SaveGame->CurrentBuild = CurrentBuild;
	SaveGame->CurrentBuild.Cards.Reset();
	SaveGame->InventoryItems = InventoryItems;
	SaveGame->OwnedPartIds = OwnedPartIds;
	SaveGame->bAutomaticAttackMode = bAutomaticAttackMode;
	// v5+ writes only the new echo storage state; RecordingHistory and AnchorId stay empty on purpose.
	SaveGame->bHasPendingRecording = bHasPendingRecording;
	if (bHasPendingRecording)
	{
		SaveGame->PendingRecording = PendingRecording;
	}
	SaveGame->bHasLatestCompletedRecording = bHasLatestCompletedRecording;
	if (bHasLatestCompletedRecording)
	{
		SaveGame->LatestCompletedRecording = LatestCompletedRecording;
	}
	SaveGame->bHasPreviousCompletedRecording = bHasPreviousCompletedRecording;
	if (bHasPreviousCompletedRecording)
	{
		SaveGame->PreviousCompletedRecording = PreviousCompletedRecording;
	}
	SaveGame->StoredEchoes = StoredEchoes;
	SaveGame->SelectedReplayIds = SelectedReplayIds;
	SaveGame->StorageCapacity = StorageCapacity;
	SaveGame->SpecificReplayLimit = SpecificReplayLimit;
	return SaveGame;
}

bool UReEchoRunSubsystem::RestoreSaveSnapshot(const UReEchoRunSaveGame& SaveGame)
{
	if (!IsValidResumableSave(SaveGame))
	{
		return false;
	}
	if (SaveGame.SaveVersion < 9 && SaveGame.EncounterIndex > 0)
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("Legacy six-encounter save version %d at encounter %d is rejected instead of being reinterpreted "
		            "as the eight-encounter run."),
		       SaveGame.SaveVersion,
		       SaveGame.EncounterIndex);
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return false;
	}
	FReEchoBuildSnapshot NormalizedCurrentBuild;
	FReEchoBuildSnapshot MigratedCurrentBuild = SaveGame.CurrentBuild;
	if (!MigrateBuildState(SaveGame.SaveVersion, *Snapshot, MigratedCurrentBuild) ||
	    !ReEchoWeaponRuntime::GetBuildConfigurationError(*Snapshot, MigratedCurrentBuild).IsEmpty() ||
	    !TryNormalizeEquipmentBuild(*Snapshot, MigratedCurrentBuild, NormalizedCurrentBuild))
	{
		return false;
	}
	TArray<FName> NormalizedOwnedParts;
	const TArray<FName> SavedOwnedParts = SaveGame.SaveVersion >= 10 ? SaveGame.OwnedPartIds : TArray<FName>{};
	if (!TryNormalizeOwnedParts(*Snapshot, SavedOwnedParts, NormalizedCurrentBuild.EquippedParts, NormalizedOwnedParts))
	{
		return false;
	}
	FReEchoEchoStorageRestoreState RestoredStorage;
	if (SaveGame.SaveVersion == 4)
	{
		MigrateV4EchoStorage(SaveGame, RestoredStorage);
	}
	else
	{
		ReadV5EchoStorage(SaveGame, RestoredStorage);
	}
	if (RestoredStorage.bHasPendingRecording &&
	    (!MigrateBuildState(SaveGame.SaveVersion, *Snapshot, RestoredStorage.PendingRecording.BuildSnapshot) ||
	     !TryNormalizeRestoredRecording(*Snapshot, RestoredStorage.PendingRecording)))
	{
		return false;
	}
	if (RestoredStorage.bHasLatestCompletedRecording &&
	    (!MigrateBuildState(SaveGame.SaveVersion, *Snapshot, RestoredStorage.LatestCompletedRecording.BuildSnapshot) ||
	     !TryNormalizeRestoredRecording(*Snapshot, RestoredStorage.LatestCompletedRecording)))
	{
		return false;
	}
	if (RestoredStorage.bHasPreviousCompletedRecording &&
	    (!MigrateBuildState(
	         SaveGame.SaveVersion, *Snapshot, RestoredStorage.PreviousCompletedRecording.BuildSnapshot) ||
	     !TryNormalizeRestoredRecording(*Snapshot, RestoredStorage.PreviousCompletedRecording)))
	{
		return false;
	}
	for (FReEchoRecording& Stored : RestoredStorage.StoredEchoes)
	{
		if (!MigrateBuildState(SaveGame.SaveVersion, *Snapshot, Stored.BuildSnapshot) ||
		    !TryNormalizeRestoredRecording(*Snapshot, Stored))
		{
			return false;
		}
	}
	FReEchoEncounterRuntimeState NormalizedEncounterRuntimeState = SaveGame.EncounterRuntimeState;
	if (SaveGame.EncounterRuntimeState.bValid)
	{
		if (!MigrateBuildState(
		        SaveGame.SaveVersion, *Snapshot, NormalizedEncounterRuntimeState.ActiveRecording.BuildSnapshot))
		{
			return false;
		}
		FString ActiveRecordingError;
		if (!ValidateRecordingAgainstSnapshot(
		        *Snapshot, NormalizedEncounterRuntimeState.ActiveRecording, ActiveRecordingError))
		{
			return false;
		}
		if (!TryNormalizeEquipmentBuild(*Snapshot,
		                                NormalizedEncounterRuntimeState.ActiveRecording.BuildSnapshot,
		                                NormalizedEncounterRuntimeState.ActiveRecording.BuildSnapshot))
		{
			return false;
		}
	}

	EncounterIndex = FMath::Max(0, SaveGame.EncounterIndex);
	TimeShards = FMath::Max(0, SaveGame.TimeShards);
	TraitOfferSeed = SaveGame.SaveVersion >= 12 && SaveGame.TraitOfferSeed != 0
	                     ? SaveGame.TraitOfferSeed
	                     : MigrateLegacyTraitOfferSeed(SaveGame);
	CurrentBuild = NormalizedCurrentBuild;
	RunDataSnapshot = Snapshot;
	InventoryItems = SaveGame.InventoryItems;
	OwnedPartIds = MoveTemp(NormalizedOwnedParts);
	bAutomaticAttackMode = SaveGame.SaveVersion >= 6 ? SaveGame.bAutomaticAttackMode : true;
	bHasPendingRecording = RestoredStorage.bHasPendingRecording;
	PendingRecording = RestoredStorage.bHasPendingRecording ? RestoredStorage.PendingRecording : FReEchoRecording{};
	bHasLatestCompletedRecording = RestoredStorage.bHasLatestCompletedRecording;
	LatestCompletedRecording =
	    RestoredStorage.bHasLatestCompletedRecording ? RestoredStorage.LatestCompletedRecording : FReEchoRecording{};
	bHasPreviousCompletedRecording = RestoredStorage.bHasPreviousCompletedRecording;
	PreviousCompletedRecording = RestoredStorage.bHasPreviousCompletedRecording
	                                 ? RestoredStorage.PreviousCompletedRecording
	                                 : FReEchoRecording{};
	StoredEchoes = MoveTemp(RestoredStorage.StoredEchoes);
	StorageCapacity = RestoredStorage.StorageCapacity;
	SpecificReplayLimit = RestoredStorage.SpecificReplayLimit;
	SelectedReplayIds = MoveTemp(RestoredStorage.SelectedReplayIds);
	NormalizeSelectedReplayIds();
	PendingTraitCardIds.Reset();
	PendingEncounterResume = NormalizedEncounterRuntimeState;
	if (SaveGame.SavedPhase != EReEchoRunPhase::Encounter)
	{
		PendingEncounterResume = {};
	}
	SetPhase(SaveGame.SavedPhase);
	return true;
}
