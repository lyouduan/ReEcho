#include "Run/ReEchoRunSubsystem.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Core/ReEchoBalanceSettings.h"
#include "ReEcho.h"
#include "Run/ReEchoCharacterPromotion.h"
#include "Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoShopCatalog.h"
#include "Cards/ReEchoCardRuntime.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr const TCHAR* TraitOfferGroup = TEXT("Trait");
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
	Offer.Tier = Card.Tier;
	return Offer;
}

FReEchoTraitCardOffer MakeTraitOffer(const FReEchoCardDefinition& Card)
{
	FReEchoTraitCardOffer Offer;
	Offer.CardId = Card.Id;
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.Description = FText::FromString(Card.Description);
	Offer.Tags = Card.Tags;
	Offer.Tier = Card.Tier;
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

FName MakeShopCardOfferId(const int32 EncounterIndex, const int32 RefreshSequence, const FName CardId)
{
	return FName(*FString::Printf(TEXT("SHOP_CARD_E%d_R%d_%s"), EncounterIndex, RefreshSequence, *CardId.ToString()));
}

FReEchoShopOffer MakeOwnedBuildCardOffer(const FReEchoCardDefinition& Card)
{
	FReEchoShopOffer Offer;
	Offer.ItemId = Card.Id;
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.EffectText = FText::FromString(Card.Description);
	Offer.Tier = FMath::Clamp(Card.Tier, 1, 3);
	Offer.Price = Offer.Tier * 10;
	Offer.Type = EReEchoShopOfferType::BuildCard;
	Offer.ContentId = Card.Id;
	Offer.IconTexturePath =
	    FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_%s.T_UI_CardIcon_%s"),
	                    *Card.Id.ToString(),
	                    *Card.Id.ToString());
	return Offer;
}

int32 BuildShopOfferSeed(const FName WeaponId, const int32 EncounterIndex, const int32 RefreshSequence)
{
	uint32 Seed = HashCombine(GetTypeHash(WeaponId), GetTypeHash(EncounterIndex));
	Seed = HashCombine(Seed, GetTypeHash(RefreshSequence));
	return static_cast<int32>(Seed);
}

// Derive the shop price-range category for a part.
// Universal/core parts (WeaponTypeId == Any) split by tags: Primordial (Core|DamageChannel only) -> Core_Primordial,
// element crystals (extra element tag) -> Core_Element, prism (RandomElement tag) -> PrismCrystal.
// Weapon-specific runes (WeaponTypeId != Any) -> Rune.
// NOTE: exact Core/Element/Prism split is a Plan67 open item pending designer confirmation; current mapping follows
// parts.csv tags.
FName DerivePartPriceCategory(const FReEchoCsvPartRow& Part)
{
	if (!Part.WeaponTypeId.IsNone() && Part.WeaponTypeId != TEXT("Any"))
	{
		return TEXT("Rune");
	}
	if (Part.Tags.Contains(TEXT("RandomElement")))
	{
		return TEXT("PrismCrystal");
	}
	if (Part.Tags.Num() > 2)
	{
		return TEXT("Core_Element");
	}
	return TEXT("Core_Primordial");
}

int32 GetShopPriceInRange(const FReEchoCsvDataSnapshot& Snapshot, FName Category, int32 Fallback, FRandomStream& Rand)
{
	if (const FReEchoCsvShopPriceRangeRow* Range = Snapshot.ShopPriceRanges.Find(Category))
	{
		if (Range->MaxPrice >= Range->MinPrice && Range->MaxPrice > 0)
		{
			return Rand.RandRange(Range->MinPrice, Range->MaxPrice);
		}
	}
	return Fallback;
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
	uint32 Seed =
	    HashCombine(GetTypeHash(SaveGame.CurrentBuild.CharacterId), GetTypeHash(SaveGame.CurrentBuild.WeaponId));
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
		    Build.CardState.Runtime.FreeShopRefreshes < 0 || Build.CardState.Runtime.ShopRefreshSequence < 0 ||
		    Build.CardState.Runtime.ShopCardOfferEncounterIndex < INDEX_NONE ||
		    Build.CardState.Runtime.ShopCardOfferRefreshSequence < INDEX_NONE ||
		    Build.CardState.Runtime.ShopCardOfferIds.Num() > ReEchoShopOfferCountPerGroup ||
		    static_cast<uint8>(Build.CardState.Runtime.EconomyPenalty) >
		        static_cast<uint8>(EReEchoCardEconomyPenalty::NoEnemyShardDrops) ||
		    (Build.CardState.Runtime.bHasAnchorRecording && !Build.CardState.Runtime.AnchorRecordingId.IsValid()))
		{
			return false;
		}
		TSet<FName> UniqueShopCardOffers;
		for (const FName CardId : Build.CardState.Runtime.ShopCardOfferIds)
		{
			const FReEchoCardDefinition* Card = Snapshot.CardCatalog->Find(CardId);
			if (!Card || !Card->bEnabled || Card->OfferGroup != TraitOfferGroup ||
			    UniqueShopCardOffers.Contains(CardId))
			{
				return false;
			}
			UniqueShopCardOffers.Add(CardId);
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
	ReEchoCharacterAbilityRuntime::ApplyStaticBuildEffects(*Snapshot, Character->Id, Result.Build.Stats);
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
	OwnedWeaponIds.Reset();
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

bool UReEchoRunSubsystem::TryEquipPurchasedPart(const FName PartId, FString& OutError)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvWeaponRow* Weapon =
	    Snapshot.IsValid() ? Snapshot->FindEnabledWeapon(CurrentBuild.WeaponId) : nullptr;
	if (!Snapshot.IsValid() || !Weapon)
	{
		OutError = TEXT("Cannot equip purchased part: weapon data is unavailable");
		return false;
	}
	if (!OwnedPartIds.Contains(PartId))
	{
		OutError = FString::Printf(TEXT("Cannot equip purchased part: part '%s' is not owned"), *PartId.ToString());
		return false;
	}
	const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(PartId);
	if (!Part || !IsPartCompatibleWithWeapon(*Snapshot, *Part, *Weapon))
	{
		OutError = FString::Printf(TEXT("Cannot equip purchased part: part '%s' is incompatible"), *PartId.ToString());
		return false;
	}

	TArray<FName> DesiredPartIds;
	DesiredPartIds.Reserve(CurrentBuild.EquippedParts.Num() + 1);
	for (const FReEchoEquippedPartSnapshot& EquippedPart : CurrentBuild.EquippedParts)
	{
		DesiredPartIds.Add(EquippedPart.PartId);
	}
	if (DesiredPartIds.Contains(PartId))
	{
		return true;
	}

	const int32 Capacity =
	    ReEchoWeaponRuntime::GetEffectiveSlotCapacity(*Snapshot, CurrentBuild, Weapon->WeaponTypeId, Part->SlotTypeId);
	if (Capacity <= 0)
	{
		OutError = FString::Printf(TEXT("Cannot equip purchased part: slot '%s' has no capacity"),
		                           *Part->SlotTypeId.ToString());
		return false;
	}

	// 同槽位已满时挤出最早装备的旧件；旧件仍在 OwnedPartIds 中，因此回落为背包库存。
	TArray<int32> SameSlotIndices;
	for (int32 Index = 0; Index < DesiredPartIds.Num(); ++Index)
	{
		const FReEchoCsvPartRow* EquippedRow = Snapshot->Parts.Find(DesiredPartIds[Index]);
		if (EquippedRow && EquippedRow->SlotTypeId == Part->SlotTypeId)
		{
			SameSlotIndices.Add(Index);
		}
	}
	while (SameSlotIndices.Num() >= Capacity)
	{
		DesiredPartIds.RemoveAt(SameSlotIndices[0]);
		SameSlotIndices.RemoveAt(0);
		for (int32& ShiftedIndex : SameSlotIndices)
		{
			--ShiftedIndex;
		}
	}
	DesiredPartIds.Add(PartId);
	return TryEquipParts(DesiredPartIds, OutError);
}

FReEchoWeaponPartShopView UReEchoRunSubsystem::GetWeaponPartShopView()
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

	// Owned parts (backpack panel).
	for (const auto& Pair : Snapshot->Parts)
	{
		const FReEchoCsvPartRow& Part = Pair.Value;
		if (OwnedPartIds.Contains(Part.PartId))
		{
			View.OwnedParts.Add(MakeWeaponPartOffer(Part));
		}
	}

	// Owned weapons (marked as 已获得 in shop).
	for (const FName WeaponId : OwnedWeaponIds)
	{
		View.OwnedWeapons.Add(WeaponId);
	}

	// ===== Weapon/Part shop: 3 fixed slots (Plan67 Step2) =====
	View.SlotOffers.SetNum(ReEchoShopOfferCountPerGroup);

	TArray<FName> EquippedPartIds;
	for (const FReEchoEquippedPartSnapshot& Eq : CurrentBuild.EquippedParts)
	{
		EquippedPartIds.Add(Eq.PartId);
	}
	const auto IsPartExcluded = [&](const FName& PartId) -> bool
	{
		return OwnedPartIds.Contains(PartId) || EquippedPartIds.Contains(PartId) || InventoryItems.Contains(PartId);
	};
	const auto IsPartCompatible = [&](const FReEchoCsvPartRow& Part) -> bool
	{
		return Weapon ? IsPartCompatibleWithWeapon(*Snapshot, Part, *Weapon) : true;
	};

	TArray<const FReEchoCsvPartRow*> UniversalRuneCandidates;     // WeaponTypeId == Any, compatible, not owned/equipped
	TArray<const FReEchoCsvPartRow*> CurrentWeaponRuneCandidates; // WeaponTypeId == current weapon, not owned
	TArray<const FReEchoCsvPartRow*> OtherWeaponRuneCandidates; // WeaponTypeId != Any and != current weapon, not owned
	TArray<const FReEchoCsvWeaponRow*> OtherWeaponCandidates;   // StartSelectable except current weapon
	for (const auto& Pair : Snapshot->Parts)
	{
		const FReEchoCsvPartRow& Part = Pair.Value;
		if (!Part.bShopEnabled)
		{
			continue;
		}
		if (Part.WeaponTypeId.IsNone() || Part.WeaponTypeId == TEXT("Any"))
		{
			if (IsPartCompatible(Part) && !IsPartExcluded(Part.Id))
			{
				UniversalRuneCandidates.Add(&Part);
			}
		}
		else if (Weapon && Part.WeaponTypeId == Weapon->WeaponTypeId)
		{
			if (!IsPartExcluded(Part.Id))
			{
				CurrentWeaponRuneCandidates.Add(&Part);
			}
		}
		else
		{
			if (!IsPartExcluded(Part.Id))
			{
				OtherWeaponRuneCandidates.Add(&Part);
			}
		}
	}
	const TArray<FReEchoCsvWeaponRow> StartWeapons = Snapshot->GetStartSelectableWeapons();
	for (const FReEchoCsvWeaponRow& CandidateWeapon : StartWeapons)
	{
		if (CandidateWeapon.Id != CurrentBuild.WeaponId && !OwnedWeaponIds.Contains(CandidateWeapon.Id))
		{
			OtherWeaponCandidates.Add(&CandidateWeapon);
		}
	}

	const auto MakePartSlotOffer = [&](const FReEchoCsvPartRow& Part, FRandomStream& R) -> FReEchoWeaponSlotOffer
	{
		FReEchoWeaponSlotOffer Offer;
		Offer.Kind = EReEchoShopOfferKind::Part;
		Offer.PartId = Part.Id;
		Offer.ItemId = Part.Id;
		Offer.ContentId = Part.Id;
		Offer.SlotTypeId = Part.SlotTypeId;
		Offer.DisplayName = FText::FromString(Part.DisplayName);
		Offer.EffectText = FText::FromString(Part.Description);
		Offer.Price = GetShopPriceInRange(*Snapshot, DerivePartPriceCategory(Part), Part.ShopPrice, R);
		return Offer;
	};
	const auto MakeWeaponSlotOffer = [&](const FReEchoCsvWeaponRow& CandidateWeapon,
	                                     FRandomStream& R) -> FReEchoWeaponSlotOffer
	{
		FReEchoWeaponSlotOffer Offer;
		Offer.Kind = EReEchoShopOfferKind::Weapon;
		Offer.WeaponId = CandidateWeapon.Id;
		Offer.ItemId = CandidateWeapon.Id;
		Offer.ContentId = CandidateWeapon.Id;
		Offer.SlotTypeId = NAME_None;
		Offer.DisplayName = FText::FromString(CandidateWeapon.DisplayName);
		Offer.EffectText = FText::Format(NSLOCTEXT("ReEcho", "WeaponSlotOfferEffect", "武器：{0}"),
		                                 FText::FromString(CandidateWeapon.DisplayName));
		Offer.Price = GetShopPriceInRange(*Snapshot, TEXT("Weapon"), 10, R);
		return Offer;
	};
	const auto PickPart = [&](const TArray<const FReEchoCsvPartRow*>& Arr, FRandomStream& R) -> const FReEchoCsvPartRow*
	{
		return Arr.Num() > 0 ? Arr[R.RandRange(0, Arr.Num() - 1)] : nullptr;
	};
	const auto PickWeapon = [&](const TArray<const FReEchoCsvWeaponRow*>& Arr,
	                            FRandomStream& R) -> const FReEchoCsvWeaponRow*
	{
		return Arr.Num() > 0 ? Arr[R.RandRange(0, Arr.Num() - 1)] : nullptr;
	};

	// Slot 0: universal rune (deterministic by seed).
	FRandomStream Slot0Rand(
	    BuildShopOfferSeed(View.WeaponId, EncounterIndex, CurrentBuild.CardState.Runtime.ShopRefreshSequence));
	if (const FReEchoCsvPartRow* Chosen = PickPart(UniversalRuneCandidates, Slot0Rand))
	{
		View.SlotOffers[0] = MakePartSlotOffer(*Chosen, Slot0Rand);
	}

	// Slots 1 & 2: weighted 70/15/15 (current-weapon rune / other weapon / other-weapon rune), with fallbacks.
	FRandomStream SlotRand(
	    BuildShopOfferSeed(TEXT("SHOP_SLOTS"), EncounterIndex, CurrentBuild.CardState.Runtime.ShopRefreshSequence));
	for (int32 SlotIndex = 1; SlotIndex <= 2; ++SlotIndex)
	{
		const float Roll = SlotRand.GetFraction();
		FReEchoWeaponSlotOffer Offer;
		if (Roll < 0.70f && CurrentWeaponRuneCandidates.Num() > 0)
		{
			Offer = MakePartSlotOffer(*PickPart(CurrentWeaponRuneCandidates, SlotRand), SlotRand);
		}
		else if (Roll < 0.85f && OtherWeaponCandidates.Num() > 0)
		{
			Offer = MakeWeaponSlotOffer(*PickWeapon(OtherWeaponCandidates, SlotRand), SlotRand);
		}
		else if (OtherWeaponRuneCandidates.Num() > 0)
		{
			Offer = MakePartSlotOffer(*PickPart(OtherWeaponRuneCandidates, SlotRand), SlotRand);
		}
		else if (CurrentWeaponRuneCandidates.Num() > 0)
		{
			Offer = MakePartSlotOffer(*PickPart(CurrentWeaponRuneCandidates, SlotRand), SlotRand);
		}
		else if (OtherWeaponCandidates.Num() > 0)
		{
			Offer = MakeWeaponSlotOffer(*PickWeapon(OtherWeaponCandidates, SlotRand), SlotRand);
		}
		View.SlotOffers[SlotIndex] = Offer; // empty if no candidates
	}

	if (Snapshot->CardCatalog.IsValid())
	{
		FReEchoCardRuntimeState& CardRuntime = CurrentBuild.CardState.Runtime;
		const int32 RefreshSequence = CardRuntime.ShopRefreshSequence;
		// Build-card shop: three fixed slots drawn from the union of the encounter's configured tiers.
		if (const FReEchoCsvShopDropLevelRow* DropLevel = Snapshot->ShopDropLevels.Find(EncounterIndex))
		{
			TArray<int32> CardTiers;
			if (!DropLevel->ShopTiers.IsEmpty())
			{
				TArray<FString> TierTokens;
				DropLevel->ShopTiers.ParseIntoArray(TierTokens, TEXT("|"));
				for (const FString& Tok : TierTokens)
				{
					const int32 Tier = FCString::Atoi(*Tok);
					if (Tier > 0)
					{
						CardTiers.Add(Tier);
					}
				}
			}
			const bool bNeedsNewCardPage = CardRuntime.ShopCardOfferEncounterIndex != EncounterIndex ||
			                               CardRuntime.ShopCardOfferRefreshSequence != RefreshSequence;
			if (bNeedsNewCardPage)
			{
				CardRuntime.ShopCardOfferEncounterIndex = EncounterIndex;
				CardRuntime.ShopCardOfferRefreshSequence = RefreshSequence;
				CardRuntime.ShopCardOfferIds.Reset();

				TArray<FReEchoCardDefinition> Eligible;
				TSet<FName> EligibleCardIds;
				for (const int32 Tier : CardTiers)
				{
					for (const FReEchoCardDefinition& Card : ReEchoCardRuntime::BuildOfferPool(
					         *Snapshot->CardCatalog, CurrentBuild.CardState, TraitOfferGroup, Tier))
					{
						if (!EligibleCardIds.Contains(Card.Id))
						{
							EligibleCardIds.Add(Card.Id);
							Eligible.Add(Card);
						}
					}
				}

				if (CardTiers.Num() > 0 && Eligible.Num() < ReEchoShopOfferCountPerGroup)
				{
					UE_LOG(
					    LogReEcho,
					    Error,
					    TEXT("Encounter %d shop tiers require %d card slots but only %d unowned cards are eligible."),
					    EncounterIndex,
					    ReEchoShopOfferCountPerGroup,
					    Eligible.Num());
				}
				else if (CardTiers.Num() > 0)
				{
					FRandomStream CardRand(BuildShopOfferSeed(TEXT("SHOP_CARDS"), EncounterIndex, RefreshSequence));
					ShuffleOffers(Eligible, CardRand);
					for (int32 SlotIndex = 0; SlotIndex < ReEchoShopOfferCountPerGroup; ++SlotIndex)
					{
						CardRuntime.ShopCardOfferIds.Add(Eligible[SlotIndex].Id);
					}
				}
			}

			for (const FName CardId : CardRuntime.ShopCardOfferIds)
			{
				const FReEchoCardDefinition* Chosen = Snapshot->CardCatalog->Find(CardId);
				if (!Chosen || !CardTiers.Contains(Chosen->Tier))
				{
					UE_LOG(LogReEcho,
					       Error,
					       TEXT("Encounter %d cached shop card '%s' no longer belongs to the configured tier pool."),
					       EncounterIndex,
					       *CardId.ToString());
					continue;
				}
				FRandomStream PriceRand(BuildShopOfferSeed(Chosen->Id, EncounterIndex, RefreshSequence));
				FReEchoCardSlotOffer CardOffer;
				CardOffer.Tier = Chosen->Tier;
				CardOffer.CardId = Chosen->Id;
				CardOffer.ItemId = MakeShopCardOfferId(EncounterIndex, RefreshSequence, Chosen->Id);
				CardOffer.DisplayName = FText::FromString(Chosen->DisplayName);
				CardOffer.EffectText = FText::FromString(Chosen->Description);
				CardOffer.bFree = false;
				CardOffer.Price = GetShopPriceInRange(
				    *Snapshot, *FString::Printf(TEXT("Card_T%d"), Chosen->Tier), Chosen->Tier * 10, PriceRand);
				View.CardSlotOffers.Add(CardOffer);
			}
		}

		for (const FName CardId : CurrentBuild.CardState.OwnedCardIds)
		{
			if (const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId))
			{
				FReEchoShopOffer OwnedCard = MakeOwnedBuildCardOffer(*Card);
				View.OwnedCards.Add(MoveTemp(OwnedCard));
			}
		}
	}

	// Backward-compatibility bridge (Plan67 Step2): flatten SlotOffers + CardSlotOffers into the legacy
	// Offers list so the existing WBP widget and automation tests keep compiling until the UI is rearranged
	// (Plan67 Step5 follow-up). Whole-weapon offers are surfaced here with Type == Weapon so the shop can sell them.
	for (const FReEchoWeaponSlotOffer& Slot : View.SlotOffers)
	{
		if (Slot.PartId.IsNone() && Slot.WeaponId.IsNone())
		{
			continue;
		}
		FReEchoShopOffer Offer;
		Offer.ItemId = Slot.ItemId;
		Offer.ContentId = Slot.PartId.IsNone() ? Slot.WeaponId : Slot.PartId;
		Offer.DisplayName = Slot.DisplayName;
		Offer.EffectText = Slot.EffectText;
		Offer.Price = Slot.Price;
		Offer.SlotTypeId = Slot.SlotTypeId;
		Offer.Type = (Slot.Kind == EReEchoShopOfferKind::Weapon) ? EReEchoShopOfferType::Weapon
		                                                         : EReEchoShopOfferType::WeaponPart;
		// 武器 Offer 携带对应配图路径(开局选武器界面同款)，渲染时优先于默认卡片图标
		if (Slot.Kind == EReEchoShopOfferKind::Weapon)
		{
			if (const FReEchoCsvWeaponRow* WeaponRow = Snapshot->FindWeapon(Slot.WeaponId))
			{
				Offer.IconTexturePath = FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(WeaponRow->VisualKey);
			}
		}
		View.Offers.Add(Offer);
	}
	for (const FReEchoCardSlotOffer& Card : View.CardSlotOffers)
	{
		FReEchoShopOffer Offer;
		Offer.ItemId = Card.ItemId;
		Offer.ContentId = Card.CardId;
		Offer.DisplayName = Card.DisplayName;
		Offer.EffectText = Card.EffectText;
		Offer.Price = Card.Price;
		Offer.Type = EReEchoShopOfferType::BuildCard;
		Offer.Tier = Card.Tier;
		Offer.IconTexturePath =
		    FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_%s.T_UI_CardIcon_%s"),
		                    *Card.CardId.ToString(),
		                    *Card.CardId.ToString());
		View.Offers.Add(Offer);
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

int32 UReEchoRunSubsystem::ResolveConfiguredFreeTraitTier() const
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid())
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("Cannot resolve post-encounter card drop for encounter %d: data snapshot is unavailable."),
		       EncounterIndex);
		return INDEX_NONE;
	}

	const FReEchoCsvShopDropLevelRow* DropLevel = Snapshot->ShopDropLevels.Find(EncounterIndex);
	if (!DropLevel)
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("Cannot resolve post-encounter card drop: encounter %d has no shop_drop_levels row."),
		       EncounterIndex);
		return INDEX_NONE;
	}
	if (DropLevel->FreeTier == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	if (DropLevel->FreeTier < 1 || DropLevel->FreeTier > 3)
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("Cannot resolve post-encounter card drop: encounter %d has invalid FreeTier %d."),
		       EncounterIndex,
		       DropLevel->FreeTier);
		return INDEX_NONE;
	}
	return DropLevel->FreeTier;
}

void UReEchoRunSubsystem::AdvanceToConfiguredTraitChoice()
{
	SetPhase(ResolveConfiguredFreeTraitTier() == INDEX_NONE ? EReEchoRunPhase::Planning : EReEchoRunPhase::CardChoice);
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
	if (Phase == EReEchoRunPhase::Planning || Phase == EReEchoRunPhase::CardChoice || Phase == EReEchoRunPhase::Shop ||
	    Phase == EReEchoRunPhase::Summary || Phase == EReEchoRunPhase::Failed)
	{
		return;
	}
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
	const TSharedPtr<const FReEchoCsvDataSnapshot> AbilitySnapshot = GetRunDataSnapshot();
	if (AbilitySnapshot.IsValid())
	{
		FReEchoBuildSnapshot Candidate;
		if (TryMutateAuthoritativeBuild(
		        *AbilitySnapshot,
		        CurrentBuild,
		        [&](FReEchoBuildSnapshot& BaseBuild)
		        {
			        ReEchoCharacterAbilityRuntime::ApplyEncounterCompletedEffects(
			            *AbilitySnapshot, BaseBuild.CharacterId, BaseBuild.Stats);
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
	AdvanceToConfiguredTraitChoice();
}

TArray<FReEchoTraitCardOffer> UReEchoRunSubsystem::GenerateTraitCardOffers(const int32 RequestedCount)
{
	PendingTraitCardIds.Reset();
	if (RequestedCount <= 0 || Phase != EReEchoRunPhase::CardChoice)
	{
		return {};
	}

	const int32 FreeTier = ResolveConfiguredFreeTraitTier();
	if (FreeTier == INDEX_NONE)
	{
		UE_LOG(LogReEcho,
		       Error,
		       TEXT("CardChoice phase has no configured free card tier for encounter %d; continuing to the shop."),
		       EncounterIndex);
		SetPhase(EReEchoRunPhase::Planning);
		return {};
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const TArray<FReEchoCardDefinition> Catalog =
	    Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()
	        ? ReEchoCardRuntime::BuildOfferPool(
	              *Snapshot->CardCatalog, CurrentBuild.CardState, TraitOfferGroup, FreeTier)
	        : TArray<FReEchoCardDefinition>();
	if (Catalog.Num() < RequestedCount)
	{
		UE_LOG(
		    LogReEcho,
		    Error,
		    TEXT("Encounter %d FreeTier %d requires %d card offers but only %d are eligible; continuing to the shop."),
		    EncounterIndex,
		    FreeTier,
		    RequestedCount,
		    Catalog.Num());
		SetPhase(EReEchoRunPhase::Planning);
		return {};
	}
	const int32 OfferCount = RequestedCount;

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

	const FName BonusTraitChoicesRemainingFlag = TEXT("BonusTraitChoicesRemaining");
	const FName NormalTraitSelectionsFlag = TEXT("NormalTraitSelections");
	const int32 ExistingBonusChoices =
	    FMath::Max(0, FCString::Atoi(*CurrentBuild.RuleFlags.FindRef(BonusTraitChoicesRemainingFlag)));
	const bool bApplyingBonusChoice = ExistingBonusChoices > 0;
	bool bContinueBonusChoices = false;
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
		        if (bApplyingBonusChoice)
		        {
			        const int32 Remaining = ExistingBonusChoices - 1;
			        bContinueBonusChoices = Remaining > 0;
			        if (bContinueBonusChoices)
			        {
				        BaseBuild.RuleFlags.Add(BonusTraitChoicesRemainingFlag, FString::FromInt(Remaining));
			        }
			        else
			        {
				        BaseBuild.RuleFlags.Remove(BonusTraitChoicesRemainingFlag);
			        }
			        return true;
		        }
		        const int32 NormalTraitSelections =
		            FCString::Atoi(*BaseBuild.RuleFlags.FindRef(NormalTraitSelectionsFlag)) + 1;
		        BaseBuild.RuleFlags.Add(NormalTraitSelectionsFlag, FString::FromInt(NormalTraitSelections));
		        const int32 NewBonusChoices = ReEchoCharacterAbilityRuntime::ResolveExtraTraitChoices(
		            *Snapshot, BaseBuild.CharacterId, NormalTraitSelections);
		        bContinueBonusChoices = NewBonusChoices > 0;
		        if (bContinueBonusChoices)
		        {
			        BaseBuild.RuleFlags.Add(BonusTraitChoicesRemainingFlag, FString::FromInt(NewBonusChoices));
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
	SetPhase(bContinueBonusChoices ? EReEchoRunPhase::CardChoice : EReEchoRunPhase::Planning);
	return true;
}

bool UReEchoRunSubsystem::DebugGrantCard(const FName CardId)
{
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[DebugGrantCard] enter: CardId=%s Phase=%d EncounterIndex=%d TimeShards=%d CurrentCards=%d"),
	       *CardId.ToString(),
	       static_cast<int32>(Phase),
	       EncounterIndex,
	       TimeShards,
	       CurrentBuild.CardState.OwnedCardIds.Num());

	if (CardId.IsNone())
	{
		UE_LOG(LogReEcho, Warning, TEXT("[DebugGrantCard] abort: CardId is None"));
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid() || !Snapshot->CardCatalog->Find(CardId))
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[DebugGrantCard] abort: snapshot valid=%d catalog valid=%d card found=%d"),
		       Snapshot.IsValid(),
		       Snapshot.IsValid() && Snapshot->CardCatalog.IsValid(),
		       Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() && Snapshot->CardCatalog->Find(CardId) != nullptr);
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
			        UE_LOG(LogReEcho,
			               Warning,
			               TEXT("[DebugGrantCard] grant failed: CardId=%s bSucceeded=%d"),
			               *CardId.ToString(),
			               Grant.bSucceeded);
			        return false;
		        }
		        UE_LOG(LogReEcho,
		               Warning,
		               TEXT("[DebugGrantCard] grant ok: CardId=%s newCards=%d"),
		               *CardId.ToString(),
		               BaseBuild.CardState.OwnedCardIds.Num());
		        BaseBuild.Stats = Grant.Stats;
		        BaseBuild.CardState = Grant.CardState;
		        ReEchoCharacterPromotion::TryPromote(BaseBuild);
		        UE_LOG(LogReEcho,
		               Warning,
		               TEXT("[DebugGrantCard] after promote: Cards=%d"),
		               BaseBuild.CardState.OwnedCardIds.Num());
		        return true;
	        },
	        PendingBuild))
	{
		UE_LOG(LogReEcho, Warning, TEXT("[DebugGrantCard] abort: TryMutateAuthoritativeBuild rejected"));
		return false;
	}
	CurrentBuild = PendingBuild;
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[DebugGrantCard] done: CardId=%s finalCards=%d"),
	       *CardId.ToString(),
	       CurrentBuild.CardState.OwnedCardIds.Num());
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

void UReEchoRunSubsystem::LogWeaponRunePurchaseState(const TSharedPtr<const FReEchoCsvDataSnapshot>& Snapshot,
                                                     FName PurchasedItem,
                                                     const TCHAR* PurchaseKind,
                                                     const FReEchoBuildSnapshot& BeforeBuild,
                                                     const FReEchoBuildSnapshot& AfterBuild,
                                                     FName AffectedSlotTypeId)
{
	if (!Snapshot)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[WeaponRuneLog] purchase=%s kind=%s | <no snapshot>"),
		       *PurchasedItem.ToString(),
		       PurchaseKind);
		return;
	}

	const FReEchoCsvWeaponRow* Weapon = Snapshot->FindEnabledWeapon(AfterBuild.WeaponId);

	// 1) 当前装备的武器
	if (Weapon)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[WeaponRuneLog] purchase=%s kind=%s | EquippedWeapon=%s (WeaponId=%s, WeaponTypeId=%s)"),
		       *PurchasedItem.ToString(),
		       PurchaseKind,
		       *Weapon->DisplayName,
		       *AfterBuild.WeaponId.ToString(),
		       *Weapon->WeaponTypeId.ToString());
	}
	else
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[WeaponRuneLog] purchase=%s kind=%s | EquippedWeapon=<none> (WeaponId=%s)"),
		       *PurchasedItem.ToString(),
		       PurchaseKind,
		       *AfterBuild.WeaponId.ToString());
		return;
	}

	const auto PartName = [&](FName Pid) -> FString
	{
		const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(Pid);
		return Part ? Part->DisplayName : Pid.ToString();
	};

	// 2) 各槽位：装备中 / 本次装备 / 本次卸下 / 背包
	for (const TPair<FName, FReEchoCsvSlotProfileRow>& ProfilePair : Snapshot->SlotProfiles)
	{
		const FReEchoCsvSlotProfileRow& Profile = ProfilePair.Value;
		if (Profile.WeaponTypeId != Weapon->WeaponTypeId)
		{
			continue;
		}
		const FName SlotTypeId = Profile.SlotTypeId;
		// 购买武器符文时只关注对应槽位；购买整把武器时打印全部槽位
		if (AffectedSlotTypeId != NAME_None && SlotTypeId != AffectedSlotTypeId)
		{
			continue;
		}

		const int32 Capacity =
		    ReEchoWeaponRuntime::GetEffectiveSlotCapacity(*Snapshot, AfterBuild, Weapon->WeaponTypeId, SlotTypeId);
		const FReEchoCsvSlotTypeRow* SlotType = Snapshot->SlotTypes.Find(SlotTypeId);
		const FString SlotName = SlotType ? SlotType->DisplayName : SlotTypeId.ToString();

		TArray<FName> EquippedBefore;
		TArray<FName> EquippedNow;
		for (const FReEchoEquippedPartSnapshot& Eq : BeforeBuild.EquippedParts)
		{
			if (Eq.SlotTypeId == SlotTypeId)
			{
				EquippedBefore.Add(Eq.PartId);
			}
		}
		for (const FReEchoEquippedPartSnapshot& Eq : AfterBuild.EquippedParts)
		{
			if (Eq.SlotTypeId == SlotTypeId)
			{
				EquippedNow.Add(Eq.PartId);
			}
		}

		TArray<FString> EquippedNames;
		TArray<FString> EquippedThisTime;
		for (const FName& Pid : EquippedNow)
		{
			EquippedNames.Add(PartName(Pid));
			if (!EquippedBefore.Contains(Pid))
			{
				EquippedThisTime.Add(PartName(Pid));
			}
		}
		TArray<FString> UnequippedThisTime;
		for (const FName& Pid : EquippedBefore)
		{
			if (!EquippedNow.Contains(Pid))
			{
				UnequippedThisTime.Add(PartName(Pid));
			}
		}

		// 对应槽位的背包：已拥有、与该武器兼容、属于该槽位、但未装备
		TArray<FString> BackpackNames;
		for (const FName& OwnedId : OwnedPartIds)
		{
			const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(OwnedId);
			if (!Part || !Part->bEnabled)
			{
				continue;
			}
			if (Part->SlotTypeId != SlotTypeId)
			{
				continue;
			}
			if (!IsPartCompatibleWithWeapon(*Snapshot, *Part, *Weapon))
			{
				continue;
			}
			if (EquippedNow.Contains(OwnedId))
			{
				continue;
			}
			BackpackNames.Add(Part->DisplayName);
		}

		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[WeaponRuneLog]   slot=%s capacity=%d | equipped[%d]=%s | unequipped=%s | backpack[%d]=%s"),
		       *SlotName,
		       Capacity,
		       EquippedNow.Num(),
		       *FString::Join(EquippedNames, TEXT(", ")),
		       *FString::Join(UnequippedThisTime, TEXT(", ")),
		       BackpackNames.Num(),
		       *FString::Join(BackpackNames, TEXT(", ")));

		if (EquippedThisTime.Num() > 0)
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[WeaponRuneLog]   EQUIP -> %s"),
			       *FString::Join(EquippedThisTime, TEXT(", ")));
		}
		if (UnequippedThisTime.Num() > 0)
		{
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[WeaponRuneLog]   UNEQUIP -> %s"),
			       *FString::Join(UnequippedThisTime, TEXT(", ")));
		}
	}
}

bool UReEchoRunSubsystem::PurchaseShopItem(const FName ItemId)
{
	const FReEchoWeaponPartShopView ShopView = GetWeaponPartShopView();

	// Locate offer across the new fixed slots + card slots (Plan67 Step2/3).
	FReEchoWeaponSlotOffer SlotOffer;
	bool bFoundSlot = false;
	for (const FReEchoWeaponSlotOffer& Candidate : ShopView.SlotOffers)
	{
		if (Candidate.ItemId == ItemId && (!Candidate.PartId.IsNone() || !Candidate.WeaponId.IsNone()))
		{
			SlotOffer = Candidate;
			bFoundSlot = true;
			break;
		}
	}
	FReEchoCardSlotOffer CardSlotOffer;
	bool bFoundCard = false;
	if (!bFoundSlot)
	{
		for (const FReEchoCardSlotOffer& Candidate : ShopView.CardSlotOffers)
		{
			if (Candidate.ItemId == ItemId)
			{
				CardSlotOffer = Candidate;
				bFoundCard = true;
				break;
			}
		}
	}

	// Legacy rune-item catalog (SHOP_RUSTED_SCISSORS etc.) kept for backward compatibility.
	EReEchoShopOfferType LegacyType = EReEchoShopOfferType::RunItem;
	int32 LegacyPrice = 0;
	bool bIsLegacy = false;
	if (!bFoundSlot && !bFoundCard)
	{
		for (const FReEchoShopOffer& Legacy : GetReEchoShopCatalog())
		{
			if (Legacy.ItemId == ItemId)
			{
				LegacyType = Legacy.Type;
				LegacyPrice = Legacy.Price;
				bIsLegacy = true;
				break;
			}
		}
	}
	if (!bFoundSlot && !bFoundCard && !bIsLegacy)
	{
		return false;
	}

	// Price + free handling.
	const bool bIsFree = bFoundCard && CardSlotOffer.bFree;
	int32 RawPrice = 0;
	if (bFoundSlot)
	{
		RawPrice = SlotOffer.Price;
	}
	else if (bFoundCard)
	{
		RawPrice = CardSlotOffer.Price;
	}
	else if (bIsLegacy)
	{
		RawPrice = LegacyPrice;
	}
	const int32 EffectivePrice = bIsFree ? 0 : GetDiscountedShopPrice(RawPrice);

	if (InventoryItems.Contains(ItemId))
	{
		return false;
	}
	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Part && OwnedPartIds.Contains(SlotOffer.PartId))
	{
		return false;
	}
	if (bFoundCard && !bIsFree && !CanPurchaseExtraShopCard())
	{
		return false;
	}
	if (bFoundCard && CardSlotOffer.Tier != 1 &&
	    ReEchoCardRuntime::HasCard(CurrentBuild.CardState, CardSlotOffer.CardId))
	{
		return false;
	}

	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[ShopPurchase] enter item=%s slot=%d card=%d free=%d price=%d effective=%d shards=%d"),
	       *ItemId.ToString(),
	       bFoundSlot ? static_cast<int32>(SlotOffer.Kind) : -1,
	       bFoundCard,
	       bIsFree,
	       RawPrice,
	       EffectivePrice,
	       TimeShards);

	if (TimeShards < EffectivePrice)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("[ShopDebug] abort insufficient item=%s timeShards=%d < effectivePrice=%d"),
		       *ItemId.ToString(),
		       TimeShards,
		       EffectivePrice);
		return false;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoBuildSnapshot OriginalBuild = CurrentBuild;
	const int32 TimeShardsBeforePurchase = TimeShards;
	int32 PendingTimeShards = TimeShards;
	FReEchoBuildSnapshot PendingBuild;
	if (!Snapshot.IsValid() ||
	    !TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        if (bFoundCard)
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
			            CardSlotOffer.CardId, EncounterIndex, BaseBuild.CardState.Runtime.ShopRefreshSequence);
			        const FReEchoCardGrantResult Grant =
			            ReEchoCardRuntime::TryGrantCard(*Snapshot->CardCatalog, CardSlotOffer.CardId, Input);
			        if (!Grant.bSucceeded)
			        {
				        return false;
			        }
			        BaseBuild.Stats = Grant.Stats;
			        BaseBuild.CardState = Grant.CardState;
			        PendingTimeShards = Grant.TimeShards;
			        UE_LOG(LogReEcho,
			               Warning,
			               TEXT("[ShopDebug] BuildCard grant item=%s inTimeShards=%d outTimeShards=%d bSucceeded=%d"),
			               *CardSlotOffer.CardId.ToString(),
			               Input.TimeShards,
			               Grant.TimeShards,
			               Grant.bSucceeded);
		        }
		        else if (bIsLegacy && LegacyType == EReEchoShopOfferType::RunItem)
		        {
			        // Legacy rune-item stat modifiers.
			        if (ItemId == TEXT("SHOP_RUSTED_SCISSORS"))
			        {
				        BaseBuild.Stats.PhysicalAttack += 2.0f;
			        }
			        else if (ItemId == TEXT("SHOP_DREAM_FRUIT"))
			        {
				        BaseBuild.Stats.HpMax += 10.0f;
			        }
			        else if (ItemId == TEXT("SHOP_BLACK_FEATHER"))
			        {
				        BaseBuild.Stats.MovementSpeed += 0.1f;
			        }
			        else if (ItemId == TEXT("SHOP_OLD_COIN"))
			        {
				        BaseBuild.Stats.EchoEfficiency += 0.1f;
			        }
			        // SHOP_REPLAY_UNLOCK: no stat change (handled below).
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

	// 原子扣费：以购买前余额 TimeShards 为基准，叠加卡牌 OnGrant 对碎片的净改变(GrantShardDelta)，
	// 结果夹紧到 >=0。防止如 G_2_15(时砂豪赌，OnGrant 清零碎片)在 grant 后余额被覆盖为 0，
	// 再减售价导致 TimeShards 变负数。
	const int32 GrantShardDelta = PendingTimeShards - TimeShards;
	PendingTimeShards = FMath::Max(0, TimeShards - EffectivePrice + GrantShardDelta);
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[ShopDebug] before deduct item=%s effectivePrice=%d pendingTimeShards=%d timeShards=%d grantDelta=%d"),
	       *ItemId.ToString(),
	       EffectivePrice,
	       PendingTimeShards,
	       TimeShards,
	       GrantShardDelta);
	TimeShards = PendingTimeShards;
	CurrentBuild = PendingBuild;
	UE_LOG(LogReEcho, Warning, TEXT("[ShopDebug] after deduct item=%s timeShards=%d"), *ItemId.ToString(), TimeShards);

	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Weapon)
	{
		// Switch weapon (Plan67 Step3): change build weapon, retain compatible equipped parts only
		// (mirrors Plan75 SelectWeaponById's "compatible core only" policy at the build level).
		CurrentBuild.WeaponId = SlotOffer.WeaponId;
		OwnedWeaponIds.Add(SlotOffer.WeaponId);
		if (Snapshot.IsValid())
		{
			if (const FReEchoCsvWeaponRow* NewWeapon = Snapshot->FindEnabledWeapon(SlotOffer.WeaponId))
			{
				TArray<FReEchoEquippedPartSnapshot> Compatible;
				for (const FReEchoEquippedPartSnapshot& Eq : CurrentBuild.EquippedParts)
				{
					if (const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(Eq.PartId))
					{
						if (IsPartCompatibleWithWeapon(*Snapshot, *Part, *NewWeapon))
						{
							Compatible.Add(Eq);
						}
					}
				}
				CurrentBuild.EquippedParts = Compatible;
			}
		}
	}
	else if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Part)
	{
		// Equip rune (购买即装): mark owned then equip into current weapon.
		if (!OwnedPartIds.Contains(SlotOffer.PartId))
		{
			OwnedPartIds.Add(SlotOffer.PartId);
		}
		FString EquipError;
		TryEquipPurchasedPart(SlotOffer.PartId, EquipError);
	}
	else if (bFoundCard || bIsLegacy)
	{
		InventoryItems.Add(ItemId);
	}

	if (ItemId == TEXT("SHOP_REPLAY_UNLOCK"))
	{
		// 一次性解锁：把指定回放槽位上限拉满（3），不修改 build 属性。
		const EReEchoEchoStorageResult LimitResult = SetSpecificReplayLimit(ReEchoEchoStorage::MaxSpecificReplayLimit);
		if (LimitResult != EReEchoEchoStorageResult::Success)
		{
			TimeShards += EffectivePrice; // 设定失败则回滚扣费
			InventoryItems.Remove(ItemId);
			CurrentBuild = OriginalBuild;
			return false;
		}
	}

	// 购买武器 / 武器符文后打印装备与背包状态（UE_LOG Warning，Shipping 包内可见）。
	if (bFoundSlot)
	{
		if (SlotOffer.Kind == EReEchoShopOfferKind::Weapon)
		{
			LogWeaponRunePurchaseState(
			    Snapshot, SlotOffer.WeaponId, TEXT("Weapon"), OriginalBuild, CurrentBuild, NAME_None);
		}
		else if (SlotOffer.Kind == EReEchoShopOfferKind::Part)
		{
			LogWeaponRunePurchaseState(
			    Snapshot, SlotOffer.PartId, TEXT("WeaponPart"), OriginalBuild, CurrentBuild, SlotOffer.SlotTypeId);
		}
	}

	return true;
}

bool UReEchoRunSubsystem::GrantTimeShards(const int32 Amount)
{
	if (Amount <= 0 || TimeShards == TNumericLimits<int32>::Max())
	{
		return false;
	}

	const int64 GrantedTotal = static_cast<int64>(TimeShards) + static_cast<int64>(Amount);
	TimeShards = static_cast<int32>(FMath::Min<int64>(GrantedTotal, TNumericLimits<int32>::Max()));
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
	TraitOfferSeed = SaveGame.SaveVersion >= 12 && SaveGame.TraitOfferSeed != 0 ? SaveGame.TraitOfferSeed
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
	SetPhase(SaveGame.SavedPhase == EReEchoRunPhase::LegacyForgeChoice ? EReEchoRunPhase::CardChoice
	                                                                   : SaveGame.SavedPhase);
	return true;
}
