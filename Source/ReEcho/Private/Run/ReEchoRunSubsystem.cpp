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
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

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

int32 MakeNewEnemyShardDropSeed()
{
	const int32 Seed = static_cast<int32>(GetTypeHash(FGuid::NewGuid()));
	return Seed != 0 ? Seed : 1;
}

int32 MigrateLegacyEnemyShardDropSeed(const UReEchoRunSaveGame& SaveGame)
{
	uint32 Seed = HashCombine(GetTypeHash(SaveGame.TraitOfferSeed), GetTypeHash(SaveGame.EncounterIndex));
	Seed = HashCombine(Seed, 0x53485244u); // "SHRD": stable migration salt.
	return Seed != 0 ? static_cast<int32>(Seed) : 1;
}

bool ResolveEnemyShardDropRange(const FReEchoCsvEnemyShardDropRow& Row,
                                const FName Archetype,
                                int32& OutMin,
                                int32& OutMax)
{
	if (Archetype == TEXT("Boss"))
	{
		return false;
	}
	if (Archetype == TEXT("Ranged"))
	{
		OutMin = Row.RangedMin;
		OutMax = Row.RangedMax;
		return true;
	}
	if (Archetype == TEXT("Elite"))
	{
		OutMin = Row.EliteMin;
		OutMax = Row.EliteMax;
		return OutMin != INDEX_NONE && OutMax != INDEX_NONE;
	}
	OutMin = Row.MeleeMin;
	OutMax = Row.MeleeMax;
	return true;
}

int32 BuildEnemyShardDropRollSeed(const int32 RunSeed,
                                  const int32 EncounterIndex,
                                  const int32 SpawnIndex,
                                  const FName Archetype)
{
	uint32 Seed = HashCombine(GetTypeHash(RunSeed), GetTypeHash(EncounterIndex));
	Seed = HashCombine(Seed, GetTypeHash(SpawnIndex));
	Seed = HashCombine(Seed, GetTypeHash(Archetype.ToString()));
	return static_cast<int32>(Seed);
}

int64 BuildEnemyShardDropKey(const int32 EncounterIndex, const int32 SpawnIndex)
{
	const uint64 Packed =
	    (static_cast<uint64>(static_cast<uint32>(EncounterIndex)) << 32) | static_cast<uint32>(SpawnIndex);
	return static_cast<int64>(Packed);
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
			if (CardId.IsNone())
			{
				continue;
			}
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

struct FReEchoShopPurchaseAuditState
{
	int32 TimeShards = 0;
	FString EquippedWeapon;
	TArray<FString> EquippedRunes;
	TArray<FString> ActiveCards;
	TArray<FString> WeaponBackpack;
	TArray<FString> RuneBackpack;
	TArray<FString> Inventory;
};

FString SanitizeShopPurchaseAuditText(FString Text)
{
	Text.ReplaceInline(TEXT("\r"), TEXT("\\r"));
	Text.ReplaceInline(TEXT("\n"), TEXT("\\n"));
	return Text;
}

void WriteShopPurchaseAuditLine(const FString& Line)
{
	static FCriticalSection AuditFileMutex;
	const FScopeLock Lock(&AuditFileMutex);
	const FString LogDirectory = FPaths::ProjectLogDir();
	IFileManager& FileManager = IFileManager::Get();
	FileManager.MakeDirectory(*LogDirectory, true);
	const FString AuditLogPath = FPaths::Combine(LogDirectory, TEXT("ShopPurchaseAudit.log"));
	const FString TimestampedLine =
	    FString::Printf(TEXT("[%s] %s%s"), *FDateTime::UtcNow().ToIso8601(), *Line, LINE_TERMINATOR);
	const bool bSaved = FFileHelper::SaveStringToFile(TimestampedLine,
	                                                  *AuditLogPath,
	                                                  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
	                                                  &FileManager,
	                                                  FILEWRITE_Append);
	UE_LOG(LogReEcho, Warning, TEXT("%s"), *Line);
	if (!bSaved)
	{
		UE_LOG(LogReEcho, Error, TEXT("[ShopPurchaseAudit] Could not append the audit file '%s'"), *AuditLogPath);
	}
}

FString DescribeAuditEntry(const FName Id, const FString& DisplayName)
{
	const FString SafeId = SanitizeShopPurchaseAuditText(Id.ToString());
	const FString SafeDisplayName = SanitizeShopPurchaseAuditText(DisplayName);
	return SafeDisplayName.IsEmpty() || SafeDisplayName == SafeId
	           ? SafeId
	           : FString::Printf(TEXT("%s(%s)"), *SafeDisplayName, *SafeId);
}

const TCHAR* GetShopPurchaseResultName(const EReEchoShopPurchaseResult Result)
{
	switch (Result)
	{
		case EReEchoShopPurchaseResult::Succeeded:
			return TEXT("Succeeded");
		case EReEchoShopPurchaseResult::OfferNotFound:
			return TEXT("OfferNotFound");
		case EReEchoShopPurchaseResult::AlreadyOwned:
			return TEXT("AlreadyOwned");
		case EReEchoShopPurchaseResult::PurchaseDisabled:
			return TEXT("PurchaseDisabled");
		case EReEchoShopPurchaseResult::InsufficientCurrency:
			return TEXT("InsufficientCurrency");
		case EReEchoShopPurchaseResult::DataUnavailable:
			return TEXT("DataUnavailable");
		case EReEchoShopPurchaseResult::GrantRejected:
			return TEXT("GrantRejected");
		case EReEchoShopPurchaseResult::MutationRejected:
			return TEXT("MutationRejected");
		case EReEchoShopPurchaseResult::WeaponSelectionRejected:
			return TEXT("WeaponSelectionRejected");
		case EReEchoShopPurchaseResult::ReplayUnlockRejected:
			return TEXT("ReplayUnlockRejected");
		default:
			return TEXT("Unknown");
	}
}

FReEchoShopPurchaseAuditState CaptureShopPurchaseAuditState(const UReEchoRunSubsystem& RunSubsystem,
                                                            const TSharedPtr<const FReEchoCsvDataSnapshot>& Snapshot)
{
	FReEchoShopPurchaseAuditState State;
	State.TimeShards = RunSubsystem.TimeShards;

	const FName EquippedWeaponId = RunSubsystem.CurrentBuild.WeaponId;
	const FReEchoCsvWeaponRow* EquippedWeapon = Snapshot.IsValid() ? Snapshot->FindWeapon(EquippedWeaponId) : nullptr;
	State.EquippedWeapon =
	    DescribeAuditEntry(EquippedWeaponId, EquippedWeapon ? EquippedWeapon->DisplayName : FString());

	TSet<FName> EquippedPartIds;
	for (const FReEchoEquippedPartSnapshot& EquippedPart : RunSubsystem.CurrentBuild.EquippedParts)
	{
		EquippedPartIds.Add(EquippedPart.PartId);
		const FReEchoCsvPartRow* Part = Snapshot.IsValid() ? Snapshot->Parts.Find(EquippedPart.PartId) : nullptr;
		const FReEchoCsvSlotTypeRow* Slot =
		    Snapshot.IsValid() ? Snapshot->SlotTypes.Find(EquippedPart.SlotTypeId) : nullptr;
		State.EquippedRunes.Add(
		    FString::Printf(TEXT("%s:%s"),
		                    *DescribeAuditEntry(EquippedPart.SlotTypeId, Slot ? Slot->DisplayName : FString()),
		                    *DescribeAuditEntry(EquippedPart.PartId, Part ? Part->DisplayName : FString())));
	}
	State.EquippedRunes.Sort();

	TMap<FName, int32> CardStackCounts;
	for (const FName CardId : RunSubsystem.CurrentBuild.CardState.OwnedCardIds)
	{
		++CardStackCounts.FindOrAdd(CardId);
	}
	for (const TPair<FName, int32>& CardStack : CardStackCounts)
	{
		const FReEchoCardDefinition* Card = Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()
		                                        ? Snapshot->CardCatalog->Find(CardStack.Key)
		                                        : nullptr;
		State.ActiveCards.Add(FString::Printf(
		    TEXT("%sx%d"), *DescribeAuditEntry(CardStack.Key, Card ? Card->DisplayName : FString()), CardStack.Value));
	}
	State.ActiveCards.Sort();

	TSet<FName> OwnedWeaponIds = RunSubsystem.OwnedWeaponIds;
	if (!EquippedWeaponId.IsNone())
	{
		OwnedWeaponIds.Add(EquippedWeaponId);
	}
	for (const FName WeaponId : OwnedWeaponIds)
	{
		const FReEchoCsvWeaponRow* Weapon = Snapshot.IsValid() ? Snapshot->FindWeapon(WeaponId) : nullptr;
		State.WeaponBackpack.Add(
		    FString::Printf(TEXT("%s%s"),
		                    *DescribeAuditEntry(WeaponId, Weapon ? Weapon->DisplayName : FString()),
		                    WeaponId == EquippedWeaponId ? TEXT("[equipped]") : TEXT("")));
	}
	State.WeaponBackpack.Sort();

	for (const FName PartId : RunSubsystem.OwnedPartIds)
	{
		if (EquippedPartIds.Contains(PartId))
		{
			continue;
		}
		const FReEchoCsvPartRow* Part = Snapshot.IsValid() ? Snapshot->Parts.Find(PartId) : nullptr;
		State.RuneBackpack.Add(DescribeAuditEntry(PartId, Part ? Part->DisplayName : FString()));
	}
	State.RuneBackpack.Sort();

	for (const FName InventoryItemId : RunSubsystem.InventoryItems)
	{
		FString DisplayName;
		for (const FReEchoShopOffer& LegacyOffer : GetReEchoShopCatalog())
		{
			if (LegacyOffer.ItemId == InventoryItemId)
			{
				DisplayName = LegacyOffer.DisplayName.ToString();
				break;
			}
		}
		State.Inventory.Add(DescribeAuditEntry(InventoryItemId, DisplayName));
	}
	State.Inventory.Sort();
	return State;
}

void LogShopPurchaseAuditState(const FString& TransactionId,
                               const FName ItemId,
                               const TCHAR* Phase,
                               const FReEchoShopPurchaseAuditState& State)
{
	WriteShopPurchaseAuditLine(FString::Printf(
	    TEXT("[ShopPurchaseAudit] tx=%s phase=%s item=%s shards=%d equippedWeapon=[%s] equippedRunes=[%s] "
	         "activeCards=[%s] weaponBackpack=[%s] runeBackpack=[%s] inventory=[%s]"),
	    *TransactionId,
	    Phase,
	    *ItemId.ToString(),
	    State.TimeShards,
	    *State.EquippedWeapon,
	    *FString::Join(State.EquippedRunes, TEXT(", ")),
	    *FString::Join(State.ActiveCards, TEXT(", ")),
	    *FString::Join(State.WeaponBackpack, TEXT(", ")),
	    *FString::Join(State.RuneBackpack, TEXT(", ")),
	    *FString::Join(State.Inventory, TEXT(", "))));
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
	EnemyShardDropSeed = MakeNewEnemyShardDropSeed();
	RewardedEnemyShardDropKeys.Reset();
	InventoryItems.Reset();
	OwnedPartIds.Reset();
	OwnedWeaponIds.Reset();
	WeaponPartShopOfferEncounterIndex = INDEX_NONE;
	WeaponPartShopOfferRefreshSequence = INDEX_NONE;
	WeaponPartShopOfferIds.Reset();
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
	OwnedWeaponIds.Add(CurrentBuild.WeaponId);
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

bool UReEchoRunSubsystem::TryEquipOwnedWeapon(const FName WeaponId, FString& OutError)
{
	if (WeaponId.IsNone() || !OwnedWeaponIds.Contains(WeaponId))
	{
		OutError = FString::Printf(TEXT("Cannot equip weapon '%s': weapon is not owned"), *WeaponId.ToString());
		return false;
	}
	if (CurrentBuild.WeaponId == WeaponId)
	{
		OutError.Reset();
		return true;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid())
	{
		OutError = TEXT("Cannot equip owned weapon: CSV snapshot is unavailable");
		return false;
	}
	FReEchoBuildSnapshot Candidate;
	if (!ReEchoWeaponRuntime::TrySelectWeapon(*Snapshot, CurrentBuild, WeaponId, Candidate, OutError))
	{
		return false;
	}
	CurrentBuild = MoveTemp(Candidate);
	OutError.Reset();
	return true;
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
	View.WeaponIconTexturePath = FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(Weapon->VisualKey);
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
		const FReEchoCsvWeaponRow* OwnedWeapon = Snapshot->FindEnabledWeapon(WeaponId);
		if (!OwnedWeapon)
		{
			continue;
		}
		View.OwnedWeapons.Add(WeaponId);
		FReEchoShopOffer OwnedOffer;
		OwnedOffer.ItemId = OwnedWeapon->Id;
		OwnedOffer.ContentId = OwnedWeapon->Id;
		OwnedOffer.DisplayName = FText::FromString(OwnedWeapon->DisplayName);
		OwnedOffer.EffectText = FText::Format(NSLOCTEXT("ReEcho", "OwnedWeaponEffect", "武器：{0}"),
		                                      FText::FromString(OwnedWeapon->DisplayName));
		OwnedOffer.Type = EReEchoShopOfferType::Weapon;
		OwnedOffer.IconTexturePath = FReEchoWeaponVisualCatalog::ResolveHeldTexturePath(OwnedWeapon->VisualKey);
		View.OwnedWeaponOffers.Add(MoveTemp(OwnedOffer));
	}
	View.OwnedWeapons.Sort(
	    [](const FName Left, const FName Right)
	    {
		    return Left.LexicalLess(Right);
	    });
	View.OwnedWeaponOffers.Sort(
	    [](const FReEchoShopOffer& Left, const FReEchoShopOffer& Right)
	    {
		    return Left.ContentId.LexicalLess(Right.ContentId);
	    });

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

	const int32 WeaponPartRefreshSequence = CurrentBuild.CardState.Runtime.ShopRefreshSequence;
	const auto MakePartSlotOffer = [&](const FReEchoCsvPartRow& Part) -> FReEchoWeaponSlotOffer
	{
		FRandomStream PriceRand(BuildShopOfferSeed(Part.Id, EncounterIndex, WeaponPartRefreshSequence));
		FReEchoWeaponSlotOffer Offer;
		Offer.Kind = EReEchoShopOfferKind::Part;
		Offer.PartId = Part.Id;
		Offer.ItemId = Part.Id;
		Offer.ContentId = Part.Id;
		Offer.SlotTypeId = Part.SlotTypeId;
		Offer.DisplayName = FText::FromString(Part.DisplayName);
		Offer.EffectText = FText::FromString(Part.Description);
		Offer.Price = GetShopPriceInRange(*Snapshot, DerivePartPriceCategory(Part), Part.ShopPrice, PriceRand);
		return Offer;
	};
	const auto MakeWeaponSlotOffer = [&](const FReEchoCsvWeaponRow& CandidateWeapon) -> FReEchoWeaponSlotOffer
	{
		FRandomStream PriceRand(BuildShopOfferSeed(CandidateWeapon.Id, EncounterIndex, WeaponPartRefreshSequence));
		FReEchoWeaponSlotOffer Offer;
		Offer.Kind = EReEchoShopOfferKind::Weapon;
		Offer.WeaponId = CandidateWeapon.Id;
		Offer.ItemId = CandidateWeapon.Id;
		Offer.ContentId = CandidateWeapon.Id;
		Offer.SlotTypeId = NAME_None;
		Offer.DisplayName = FText::FromString(CandidateWeapon.DisplayName);
		Offer.EffectText = FText::Format(NSLOCTEXT("ReEcho", "WeaponSlotOfferEffect", "武器：{0}"),
		                                 FText::FromString(CandidateWeapon.DisplayName));
		Offer.Price = GetShopPriceInRange(*Snapshot, TEXT("Weapon"), 10, PriceRand);
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

	const auto IsCachedWeaponPartOfferValid = [&](const FName OfferId)
	{
		if (OfferId.IsNone())
		{
			return true;
		}
		if (const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(OfferId))
		{
			return Part->bEnabled && Part->bShopEnabled;
		}
		if (const FReEchoCsvWeaponRow* CachedWeapon = Snapshot->FindEnabledWeapon(OfferId))
		{
			return CachedWeapon->bStartSelectable;
		}
		return false;
	};
	const bool bCachedPageShapeValid = WeaponPartShopOfferIds.Num() == ReEchoShopOfferCountPerGroup &&
	                                   !WeaponPartShopOfferIds.ContainsByPredicate(
	                                       [&](const FName OfferId)
	                                       {
		                                       return !IsCachedWeaponPartOfferValid(OfferId);
	                                       });
	const bool bNeedsNewWeaponPartPage = WeaponPartShopOfferEncounterIndex != EncounterIndex ||
	                                     WeaponPartShopOfferRefreshSequence != WeaponPartRefreshSequence ||
	                                     !bCachedPageShapeValid;
	if (bNeedsNewWeaponPartPage)
	{
		WeaponPartShopOfferEncounterIndex = EncounterIndex;
		WeaponPartShopOfferRefreshSequence = WeaponPartRefreshSequence;
		WeaponPartShopOfferIds.Init(NAME_None, ReEchoShopOfferCountPerGroup);

		// Slot 0: universal rune (deterministic by seed).
		FRandomStream Slot0Rand(BuildShopOfferSeed(View.WeaponId, EncounterIndex, WeaponPartRefreshSequence));
		if (const FReEchoCsvPartRow* Chosen = PickPart(UniversalRuneCandidates, Slot0Rand))
		{
			WeaponPartShopOfferIds[0] = Chosen->Id;
		}

		// Slots 1 & 2: weighted 70/15/15 with fallbacks. Remove each result so one page never duplicates an item.
		FRandomStream SlotRand(BuildShopOfferSeed(TEXT("SHOP_SLOTS"), EncounterIndex, WeaponPartRefreshSequence));
		for (int32 SlotIndex = 1; SlotIndex <= 2; ++SlotIndex)
		{
			const float Roll = SlotRand.GetFraction();
			FName ChosenId = NAME_None;
			if (Roll < 0.70f && CurrentWeaponRuneCandidates.Num() > 0)
			{
				ChosenId = PickPart(CurrentWeaponRuneCandidates, SlotRand)->Id;
			}
			else if (Roll < 0.85f && OtherWeaponCandidates.Num() > 0)
			{
				ChosenId = PickWeapon(OtherWeaponCandidates, SlotRand)->Id;
			}
			else if (OtherWeaponRuneCandidates.Num() > 0)
			{
				ChosenId = PickPart(OtherWeaponRuneCandidates, SlotRand)->Id;
			}
			else if (CurrentWeaponRuneCandidates.Num() > 0)
			{
				ChosenId = PickPart(CurrentWeaponRuneCandidates, SlotRand)->Id;
			}
			else if (OtherWeaponCandidates.Num() > 0)
			{
				ChosenId = PickWeapon(OtherWeaponCandidates, SlotRand)->Id;
			}
			WeaponPartShopOfferIds[SlotIndex] = ChosenId;
			CurrentWeaponRuneCandidates.RemoveAll(
			    [&](const FReEchoCsvPartRow* Candidate)
			    {
				    return Candidate && Candidate->Id == ChosenId;
			    });
			OtherWeaponRuneCandidates.RemoveAll(
			    [&](const FReEchoCsvPartRow* Candidate)
			    {
				    return Candidate && Candidate->Id == ChosenId;
			    });
			OtherWeaponCandidates.RemoveAll(
			    [&](const FReEchoCsvWeaponRow* Candidate)
			    {
				    return Candidate && Candidate->Id == ChosenId;
			    });
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < WeaponPartShopOfferIds.Num(); ++SlotIndex)
	{
		const FName OfferId = WeaponPartShopOfferIds[SlotIndex];
		if (const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(OfferId))
		{
			View.SlotOffers[SlotIndex] = MakePartSlotOffer(*Part);
		}
		else if (const FReEchoCsvWeaponRow* CachedWeapon = Snapshot->FindEnabledWeapon(OfferId))
		{
			View.SlotOffers[SlotIndex] = MakeWeaponSlotOffer(*CachedWeapon);
		}
	}

	View.CardSlotOffers.SetNum(ReEchoShopOfferCountPerGroup);
	for (int32 SlotIndex = 0; SlotIndex < View.CardSlotOffers.Num(); ++SlotIndex)
	{
		FReEchoCardSlotOffer& CardSlot = View.CardSlotOffers[SlotIndex];
		CardSlot.Tier = SlotIndex + 1;
		CardSlot.DisplayName = NSLOCTEXT("ReEcho", "ShopCardTierNotOffered", "未投放");
		CardSlot.EffectText = NSLOCTEXT("ReEcho", "ShopCardTierNotOfferedDetail", "本关不投放该等级卡牌");
	}

	if (Snapshot->CardCatalog.IsValid())
	{
		FReEchoCardRuntimeState& CardRuntime = CurrentBuild.CardState.Runtime;
		const int32 RefreshSequence = CardRuntime.ShopRefreshSequence;
		// Build-card shop: fixed [Tier1, Tier2, Tier3] slots. ShopTiers only enables matching slots.
		if (const FReEchoCsvShopDropLevelRow* DropLevel = Snapshot->ShopDropLevels.Find(EncounterIndex))
		{
			TSet<int32> ConfiguredCardTiers;
			if (!DropLevel->ShopTiers.IsEmpty())
			{
				TArray<FString> TierTokens;
				DropLevel->ShopTiers.ParseIntoArray(TierTokens, TEXT("|"));
				for (const FString& Tok : TierTokens)
				{
					const int32 Tier = FCString::Atoi(*Tok);
					if (Tier >= 1 && Tier <= ReEchoShopOfferCountPerGroup)
					{
						ConfiguredCardTiers.Add(Tier);
					}
				}
			}
			const auto IsCachedCardPageValid = [&]()
			{
				if (CardRuntime.ShopCardOfferIds.Num() != ReEchoShopOfferCountPerGroup)
				{
					return false;
				}
				for (int32 SlotIndex = 0; SlotIndex < ReEchoShopOfferCountPerGroup; ++SlotIndex)
				{
					const int32 Tier = SlotIndex + 1;
					const FName CardId = CardRuntime.ShopCardOfferIds[SlotIndex];
					if (!ConfiguredCardTiers.Contains(Tier))
					{
						if (!CardId.IsNone())
						{
							return false;
						}
						continue;
					}
					if (CardId.IsNone())
					{
						continue;
					}
					const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId);
					if (!Card || !Card->bEnabled || Card->OfferGroup != TraitOfferGroup || Card->Tier != Tier)
					{
						return false;
					}
				}
				return true;
			};
			const bool bNeedsNewCardPage = CardRuntime.ShopCardOfferEncounterIndex != EncounterIndex ||
			                               CardRuntime.ShopCardOfferRefreshSequence != RefreshSequence ||
			                               !IsCachedCardPageValid();
			if (bNeedsNewCardPage)
			{
				CardRuntime.ShopCardOfferEncounterIndex = EncounterIndex;
				CardRuntime.ShopCardOfferRefreshSequence = RefreshSequence;
				CardRuntime.ShopCardOfferIds.Init(NAME_None, ReEchoShopOfferCountPerGroup);

				for (int32 SlotIndex = 0; SlotIndex < ReEchoShopOfferCountPerGroup; ++SlotIndex)
				{
					const int32 Tier = SlotIndex + 1;
					if (!ConfiguredCardTiers.Contains(Tier))
					{
						continue;
					}
					TArray<FReEchoCardDefinition> Eligible = ReEchoCardRuntime::BuildOfferPool(
					    *Snapshot->CardCatalog, CurrentBuild.CardState, TraitOfferGroup, Tier);
					if (Eligible.IsEmpty())
					{
						UE_LOG(LogReEcho,
						       Warning,
						       TEXT("Encounter %d shop tier %d has no eligible card; its fixed slot remains empty."),
						       EncounterIndex,
						       Tier);
						continue;
					}
					const FName TierSeedKey(*FString::Printf(TEXT("SHOP_CARD_TIER_%d"), Tier));
					FRandomStream CardRand(BuildShopOfferSeed(TierSeedKey, EncounterIndex, RefreshSequence));
					ShuffleOffers(Eligible, CardRand);
					CardRuntime.ShopCardOfferIds[SlotIndex] = Eligible[0].Id;
				}
			}

			for (int32 SlotIndex = 0; SlotIndex < ReEchoShopOfferCountPerGroup; ++SlotIndex)
			{
				FReEchoCardSlotOffer& CardOffer = View.CardSlotOffers[SlotIndex];
				const int32 Tier = SlotIndex + 1;
				const FName CardId = CardRuntime.ShopCardOfferIds[SlotIndex];
				if (CardId.IsNone())
				{
					if (ConfiguredCardTiers.Contains(Tier))
					{
						CardOffer.DisplayName = NSLOCTEXT("ReEcho", "ShopCardTierSoldOut", "售罄");
						CardOffer.EffectText = NSLOCTEXT("ReEcho", "ShopCardTierSoldOutDetail", "该等级暂无可购买卡牌");
					}
					continue;
				}
				const FReEchoCardDefinition* Chosen = Snapshot->CardCatalog->Find(CardId);
				if (!Chosen || Chosen->Tier != Tier)
				{
					UE_LOG(LogReEcho,
					       Error,
					       TEXT("Encounter %d cached shop card '%s' no longer matches fixed tier slot %d."),
					       EncounterIndex,
					       *CardId.ToString(),
					       Tier);
					continue;
				}
				FRandomStream PriceRand(BuildShopOfferSeed(Chosen->Id, EncounterIndex, RefreshSequence));
				CardOffer.bAvailable = true;
				CardOffer.CardId = Chosen->Id;
				CardOffer.ItemId = MakeShopCardOfferId(EncounterIndex, RefreshSequence, Chosen->Id);
				CardOffer.DisplayName = FText::FromString(Chosen->DisplayName);
				CardOffer.EffectText = FText::FromString(Chosen->Description);
				CardOffer.bFree = false;
				CardOffer.Price = GetShopPriceInRange(
				    *Snapshot, *FString::Printf(TEXT("Card_T%d"), Chosen->Tier), Chosen->Tier * 10, PriceRand);
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
		if (Card.bAvailable)
		{
			Offer.IconTexturePath =
			    FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_%s.T_UI_CardIcon_%s"),
			                    *Card.CardId.ToString(),
			                    *Card.CardId.ToString());
		}
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

FReEchoShopPurchaseOutcome UReEchoRunSubsystem::PurchaseShopItemDetailed(const FName ItemId)
{
	const FString TransactionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoShopPurchaseAuditState BeforeState = CaptureShopPurchaseAuditState(*this, Snapshot);
	LogShopPurchaseAuditState(TransactionId, ItemId, TEXT("BEFORE"), BeforeState);

	const auto FinishPurchase =
	    [&](const EReEchoShopPurchaseResult Result, const FString& Detail, const int32 EffectivePrice = 0)
	{
		FReEchoShopPurchaseOutcome Outcome;
		Outcome.TransactionId = TransactionId;
		Outcome.ItemId = ItemId;
		Outcome.Result = Result;
		Outcome.Detail = Detail;
		Outcome.EffectivePrice = EffectivePrice;
		WriteShopPurchaseAuditLine(FString::Printf(
		    TEXT("[ShopPurchaseAudit] tx=%s phase=RESULT item=%s success=%d code=%s effectivePrice=%d detail=%s"),
		    *TransactionId,
		    *ItemId.ToString(),
		    Outcome.IsSuccess(),
		    GetShopPurchaseResultName(Result),
		    EffectivePrice,
		    *SanitizeShopPurchaseAuditText(Detail)));
		LogShopPurchaseAuditState(
		    TransactionId, ItemId, TEXT("AFTER"), CaptureShopPurchaseAuditState(*this, GetRunDataSnapshot()));
		return Outcome;
	};

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
			if (Candidate.bAvailable && !ItemId.IsNone() && Candidate.ItemId == ItemId)
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
		return FinishPurchase(EReEchoShopPurchaseResult::OfferNotFound,
		                      TEXT("The requested item is not present on the current shop page"));
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
		return FinishPurchase(EReEchoShopPurchaseResult::AlreadyOwned,
		                      TEXT("The requested shop item was already purchased"),
		                      EffectivePrice);
	}
	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Part && OwnedPartIds.Contains(SlotOffer.PartId))
	{
		return FinishPurchase(
		    EReEchoShopPurchaseResult::AlreadyOwned, TEXT("The requested rune is already owned"), EffectivePrice);
	}
	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Weapon && OwnedWeaponIds.Contains(SlotOffer.WeaponId))
	{
		return FinishPurchase(
		    EReEchoShopPurchaseResult::AlreadyOwned, TEXT("The requested weapon is already owned"), EffectivePrice);
	}
	if (bFoundCard && !bIsFree && !CanPurchaseExtraShopCard())
	{
		return FinishPurchase(EReEchoShopPurchaseResult::PurchaseDisabled,
		                      TEXT("The current card rules disable extra shop-card purchases"),
		                      EffectivePrice);
	}
	if (bFoundCard && CardSlotOffer.Tier != 1 &&
	    ReEchoCardRuntime::HasCard(CurrentBuild.CardState, CardSlotOffer.CardId))
	{
		return FinishPurchase(EReEchoShopPurchaseResult::AlreadyOwned,
		                      TEXT("Owned tier-2 and tier-3 cards cannot be purchased again"),
		                      EffectivePrice);
	}

	if (TimeShards < EffectivePrice)
	{
		return FinishPurchase(
		    EReEchoShopPurchaseResult::InsufficientCurrency,
		    FString::Printf(TEXT("Time shards %d are below the effective price %d"), TimeShards, EffectivePrice),
		    EffectivePrice);
	}

	if (!Snapshot.IsValid())
	{
		return FinishPurchase(EReEchoShopPurchaseResult::DataUnavailable,
		                      TEXT("The runtime CSV snapshot is unavailable"),
		                      EffectivePrice);
	}
	if (bFoundCard && !Snapshot->CardCatalog.IsValid())
	{
		return FinishPurchase(EReEchoShopPurchaseResult::DataUnavailable,
		                      TEXT("The runtime card catalog is unavailable"),
		                      EffectivePrice);
	}

	const FReEchoBuildSnapshot OriginalBuild = CurrentBuild;
	const int32 OriginalTimeShards = TimeShards;
	const TArray<FName> OriginalInventoryItems = InventoryItems;
	const TArray<FName> OriginalOwnedPartIds = OwnedPartIds;
	const TSet<FName> OriginalOwnedWeaponIds = OwnedWeaponIds;
	int32 PendingTimeShards = TimeShards;
	FReEchoBuildSnapshot PendingBuild;
	EReEchoShopPurchaseResult MutationFailure = EReEchoShopPurchaseResult::MutationRejected;
	FString MutationFailureDetail = TEXT("The authoritative build rejected the purchase mutation");
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        if (bFoundCard)
		        {
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
				        MutationFailure = EReEchoShopPurchaseResult::GrantRejected;
				        MutationFailureDetail =
				            Grant.Error.IsEmpty() ? TEXT("The card grant was rejected") : Grant.Error;
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
		return FinishPurchase(MutationFailure, MutationFailureDetail, EffectivePrice);
	}
	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Weapon)
	{
		FReEchoBuildSnapshot SelectedWeaponBuild;
		FString SelectError;
		if (!Snapshot.IsValid() || !ReEchoWeaponRuntime::TrySelectWeapon(
		                               *Snapshot, PendingBuild, SlotOffer.WeaponId, SelectedWeaponBuild, SelectError))
		{
			return FinishPurchase(EReEchoShopPurchaseResult::WeaponSelectionRejected,
			                      SelectError.IsEmpty() ? TEXT("The purchased weapon could not be selected")
			                                            : SelectError,
			                      EffectivePrice);
		}
		PendingBuild = MoveTemp(SelectedWeaponBuild);
	}

	// 原子扣费：以购买前余额 TimeShards 为基准，叠加卡牌 OnGrant 对碎片的净改变(GrantShardDelta)，
	// 结果夹紧到 >=0。防止如 G_2_15(时砂豪赌，OnGrant 清零碎片)在 grant 后余额被覆盖为 0，
	// 再减售价导致 TimeShards 变负数。
	const int32 GrantShardDelta = PendingTimeShards - TimeShards;
	PendingTimeShards = FMath::Max(0, TimeShards - EffectivePrice + GrantShardDelta);
	TimeShards = PendingTimeShards;
	CurrentBuild = PendingBuild;
	FString CompletionDetail = TEXT("Purchase committed");

	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Weapon)
	{
		// PendingBuild already switched through WeaponRuntime so revisions, equipment base stats and compatible-rune
		// retention share the same transaction used by backpack weapon selection.
		OwnedWeaponIds.Add(SlotOffer.WeaponId);
	}
	else if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Part)
	{
		// Equip rune (购买即装): mark owned then equip into current weapon.
		if (!OwnedPartIds.Contains(SlotOffer.PartId))
		{
			OwnedPartIds.Add(SlotOffer.PartId);
		}
		FString EquipError;
		if (!TryEquipPurchasedPart(SlotOffer.PartId, EquipError))
		{
			CompletionDetail =
			    FString::Printf(TEXT("Purchase committed; automatic rune equip was rejected: %s"), *EquipError);
		}
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
			TimeShards = OriginalTimeShards;
			CurrentBuild = OriginalBuild;
			InventoryItems = OriginalInventoryItems;
			OwnedPartIds = OriginalOwnedPartIds;
			OwnedWeaponIds = OriginalOwnedWeaponIds;
			return FinishPurchase(
			    EReEchoShopPurchaseResult::ReplayUnlockRejected,
			    TEXT("The replay-selection limit could not be unlocked; the purchase was rolled back"),
			    EffectivePrice);
		}
	}

	return FinishPurchase(EReEchoShopPurchaseResult::Succeeded, CompletionDetail, EffectivePrice);
}

bool UReEchoRunSubsystem::PurchaseShopItem(const FName ItemId)
{
	return PurchaseShopItemDetailed(ItemId).IsSuccess();
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

int32 UReEchoRunSubsystem::ResolveEnemyDeathTimeShardDrop(const FName EnemyId, const int32 SpawnIndex)
{
	if (EncounterIndex <= 0 || SpawnIndex <= 0)
	{
		return 0;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvEnemyRow* Enemy = Snapshot.IsValid() ? Snapshot->FindEnabledEnemy(EnemyId) : nullptr;
	const FReEchoCsvEnemyShardDropRow* Drop =
	    Snapshot.IsValid() ? Snapshot->FindEnemyShardDrop(EncounterIndex) : nullptr;
	if (!Enemy || !Drop)
	{
		return 0;
	}
	int32 Minimum = 0;
	int32 Maximum = 0;
	if (!ResolveEnemyShardDropRange(*Drop, Enemy->Archetype, Minimum, Maximum))
	{
		return 0;
	}
	const int64 RewardKey = BuildEnemyShardDropKey(EncounterIndex, SpawnIndex);
	if (RewardedEnemyShardDropKeys.Contains(RewardKey))
	{
		return 0;
	}
	RewardedEnemyShardDropKeys.Add(RewardKey);
	if (GetCardRules().bDisableEnemyShardDrops)
	{
		return 0;
	}
	FRandomStream Random(BuildEnemyShardDropRollSeed(EnemyShardDropSeed, EncounterIndex, SpawnIndex, Enemy->Archetype));
	int32 Reward = Random.RandRange(Minimum, Maximum);
	if (CurrentBuild.CardState.Runtime.BonusShardDropEncounterIndex == EncounterIndex)
	{
		Reward = FMath::RoundToInt(static_cast<float>(Reward) * 1.5f);
	}
	return Reward;
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
	SaveGame->EnemyShardDropSeed = EnemyShardDropSeed;
	SaveGame->RewardedEnemyShardDropKeys = RewardedEnemyShardDropKeys.Array();
	SaveGame->RewardedEnemyShardDropKeys.Sort();
	SaveGame->CurrentBuild = CurrentBuild;
	SaveGame->CurrentBuild.Cards.Reset();
	SaveGame->InventoryItems = InventoryItems;
	SaveGame->OwnedPartIds = OwnedPartIds;
	for (const FName WeaponId : OwnedWeaponIds)
	{
		SaveGame->OwnedWeaponIds.Add(WeaponId);
	}
	SaveGame->OwnedWeaponIds.Sort(
	    [](const FName Left, const FName Right)
	    {
		    return Left.LexicalLess(Right);
	    });
	SaveGame->WeaponPartShopOfferEncounterIndex = WeaponPartShopOfferEncounterIndex;
	SaveGame->WeaponPartShopOfferRefreshSequence = WeaponPartShopOfferRefreshSequence;
	SaveGame->WeaponPartShopOfferIds = WeaponPartShopOfferIds;
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
	TSet<FName> NormalizedOwnedWeapons;
	if (SaveGame.SaveVersion >= 13)
	{
		for (const FName WeaponId : SaveGame.OwnedWeaponIds)
		{
			if (Snapshot->FindEnabledWeapon(WeaponId))
			{
				NormalizedOwnedWeapons.Add(WeaponId);
			}
		}
	}
	// The equipped weapon is always owned. This also migrates v12 and older saves that had no weapon backpack field.
	NormalizedOwnedWeapons.Add(NormalizedCurrentBuild.WeaponId);
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
	EnemyShardDropSeed = SaveGame.SaveVersion >= 13 && SaveGame.EnemyShardDropSeed != 0
	                         ? SaveGame.EnemyShardDropSeed
	                         : MigrateLegacyEnemyShardDropSeed(SaveGame);
	RewardedEnemyShardDropKeys.Reset();
	if (SaveGame.SaveVersion >= 13)
	{
		for (const int64 RewardKey : SaveGame.RewardedEnemyShardDropKeys)
		{
			RewardedEnemyShardDropKeys.Add(RewardKey);
		}
	}
	CurrentBuild = NormalizedCurrentBuild;
	RunDataSnapshot = Snapshot;
	InventoryItems = SaveGame.InventoryItems;
	OwnedPartIds = MoveTemp(NormalizedOwnedParts);
	OwnedWeaponIds = MoveTemp(NormalizedOwnedWeapons);
	if (SaveGame.SaveVersion >= 14)
	{
		WeaponPartShopOfferEncounterIndex = SaveGame.WeaponPartShopOfferEncounterIndex;
		WeaponPartShopOfferRefreshSequence = SaveGame.WeaponPartShopOfferRefreshSequence;
		WeaponPartShopOfferIds = SaveGame.WeaponPartShopOfferIds;
	}
	else
	{
		WeaponPartShopOfferEncounterIndex = INDEX_NONE;
		WeaponPartShopOfferRefreshSequence = INDEX_NONE;
		WeaponPartShopOfferIds.Reset();
	}
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
