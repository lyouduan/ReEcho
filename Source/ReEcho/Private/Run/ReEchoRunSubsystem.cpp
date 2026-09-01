#include "Run/ReEchoRunSubsystem.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Core/ReEchoBalanceSettings.h"
#include "Diagnostics/ReEchoBuildTrace.h"
#include "ReEcho.h"
#include "Run/CharacterAbilities/ReEchoCharacterAbilityRuntime.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoPlayerProgressSaveGame.h"
#include "Run/ReEchoShopCatalog.h"
#include "Run/ReEchoRuneInventory.h"
#include "Cards/ReEchoCardRuntime.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoCombatTarget.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

namespace
{
constexpr const TCHAR* TraitOfferGroup = TEXT("Trait");
constexpr const TCHAR* EasterEggOfferGroup = TEXT("EasterEgg");
/** Egao Party: easter-egg cards are the point of this mode, so they appear ten times as often. */
constexpr float EasterEggOfferChance = 0.10f;


const FName AnyWeaponTypeId = TEXT("Any");
const FName BonusTraitChoicesRemainingFlag = TEXT("BonusTraitChoicesRemaining");
const FName NormalTraitSelectionsFlag = TEXT("NormalTraitSelections");
const FName BossPhase3FinalStatMultiplierFlag = TEXT("BossPhase3FinalStatMultiplier");

EReEchoHealthAdjustment MergeHealthAdjustments(const EReEchoHealthAdjustment Current,
                                               const EReEchoHealthAdjustment Incoming)
{
	if (Current == EReEchoHealthAdjustment::FillToMax || Incoming == EReEchoHealthAdjustment::FillToMax)
	{
		return EReEchoHealthAdjustment::FillToMax;
	}
	if (Current == EReEchoHealthAdjustment::SetToStatPoint || Incoming == EReEchoHealthAdjustment::SetToStatPoint)
	{
		return EReEchoHealthAdjustment::SetToStatPoint;
	}
	return EReEchoHealthAdjustment::None;
}

bool DidCommittedHealthChange(const FReEchoStatBlock& Before, const FReEchoStatBlock& After)
{
	return !FMath::IsNearlyEqual(Before.HpMax, After.HpMax) || !FMath::IsNearlyEqual(Before.HpPoint, After.HpPoint);
}

const FString RunSaveSlot = TEXT("ReEchoRun");
const FString PlayerProgressSaveSlot = TEXT("ReEchoPlayerProgress");
constexpr int32 RunSaveUserIndex = 0;

FString MakeRunSaveSlotName(const int32 SlotIndex)
{
	return FString::Printf(TEXT("ReEchoRunSlot%d"), SlotIndex + 1);
}

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
	Offer.PresentationTier = Card.Tier;
	return Offer;
}

FReEchoTraitCardOffer MakeTraitOffer(const FReEchoCardDefinition& Card, const int32 PackTier = INDEX_NONE)
{
	FReEchoTraitCardOffer Offer;
	Offer.CardId = Card.Id;
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.Description = FText::FromString(Card.Description);
	Offer.Tags = Card.Tags;
	Offer.Tier = PackTier == INDEX_NONE ? Card.Tier : PackTier;
	// Easter cards have no gameplay tier, but deliberately reuse the tier-three presentation.
	Offer.PresentationTier = Card.OfferGroup == EasterEggOfferGroup ? 3 : Offer.Tier;
	return Offer;
}

bool IsCardCompatibleWithPackTier(const FReEchoCardDefinition& Card, const int32 PackTier)
{
	return (Card.OfferGroup == TraitOfferGroup && Card.Tier == PackTier) || Card.OfferGroup == EasterEggOfferGroup;
}

FName SelectCardForOfferSlot(const FReEchoCardCatalog& Catalog,
                             const FReEchoCardBuildState& State,
                             const int32 NormalTier,
                             const int32 EncounterIndex,
                             const TArray<FName>& History,
                             const int32 Seed,
                             const bool bExcludeOwnedNormalCards = false)
{
	return ReEchoCardRuntime::SelectOfferForSlot(
	    Catalog, State, NormalTier, EncounterIndex, History, Seed, EasterEggOfferChance, bExcludeOwnedNormalCards);
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
	Offer.WeaponTypeId = Part.WeaponTypeId;
	return Offer;
}

int32 RecordNormalTraitGroupSelection(const FReEchoCsvDataSnapshot& Snapshot,
                                      FReEchoBuildSnapshot& Build,
                                      const int32 CardPackTier)
{
	// Tier-gated cadence abilities (for example the Sage counting only non-tier-1 card packs) must not be
	// advanced by packs below their configured tier, so those packs leave the counter untouched.
	if (!ReEchoCharacterAbilityRuntime::DoesCardPackTierAdvanceTraitBonus(Snapshot, Build.CharacterId, CardPackTier))
	{
		return 0;
	}
	const int32 NormalTraitSelections =
	    FMath::Max(0, FCString::Atoi(*Build.RuleFlags.FindRef(NormalTraitSelectionsFlag))) + 1;
	Build.RuleFlags.Add(NormalTraitSelectionsFlag, FString::FromInt(NormalTraitSelections));
	// Cadence abilities no longer queue a deferred "extra choice". That follow-up state lived in RuleFlags,
	// survived the encounter boundary, and popped an empty, un-interactable card choice during combat. The
	// bonus is now granted up front as extra picks inside the qualifying pack itself (see
	// ResolvePackSelectableCardCount). Clear any legacy value so old saves stop carrying it.
	Build.RuleFlags.Remove(BonusTraitChoicesRemainingFlag);
	return 0;
}

FName MakeShopCardOfferId(const int32 EncounterIndex, const int32 RefreshSequence, const FName CardId)
{
	return FName(*FString::Printf(TEXT("SHOP_CARD_E%d_R%d_%s"), EncounterIndex, RefreshSequence, *CardId.ToString()));
}

FName MakeShopCardPackOfferId(const int32 EncounterIndex, const int32 RefreshSequence, const int32 Tier)
{
	return FName(*FString::Printf(TEXT("SHOP_CARD_PACK_E%d_R%d_T%d"), EncounterIndex, RefreshSequence, Tier));
}

FString FormatOutcomeValue(const FName Target, const float Value)
{
	if (Target == TEXT("ReactionEfficiency") || Target == TEXT("EchoEfficiency") || Target == TEXT("CriticalRate") ||
	    Target == TEXT("CriticalEffect") || Target == TEXT("CoreCollection") || Target == TEXT("PlayerDamage") ||
	    Target == TEXT("EchoDamage") || Target == TEXT("DamageMultiplier"))
	{
		return FString::Printf(TEXT("%+.0f%%"), Value * 100.0f);
	}
	return FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value))
	           ? FString::Printf(TEXT("%+d"), FMath::RoundToInt(Value))
	           : FString::Printf(TEXT("%+.1f"), Value);
}

FString GetOutcomeTargetLabel(const FName Target)
{
	if (Target == TEXT("PhysicalAttack"))
	{
		return TEXT("物理攻击力");
	}
	if (Target == TEXT("ElementalAttack"))
	{
		return TEXT("元素攻击力");
	}
	if (Target == TEXT("EnemyAttack"))
	{
		return TEXT("所有怪物攻击");
	}
	if (Target == TEXT("HpMax"))
	{
		return TEXT("生命上限");
	}
	if (Target == TEXT("HpMaxAndPoint"))
	{
		return TEXT("生命值与生命上限");
	}
	if (Target == TEXT("HpPoint"))
	{
		return TEXT("当前生命值");
	}
	if (Target == TEXT("CriticalRate"))
	{
		return TEXT("暴击率");
	}
	if (Target == TEXT("CriticalEffect"))
	{
		return TEXT("暴击效果");
	}
	if (Target == TEXT("TimeShardMultiplier"))
	{
		return TEXT("时间碎片余额倍率");
	}
	if (Target == TEXT("EncounterGrossTimeShards"))
	{
		return TEXT("本关正向碎片总收入");
	}
	if (Target == TEXT("NextShardIncomeMultiplier"))
	{
		return TEXT("下一关碎片收入倍率");
	}
	if (Target == TEXT("ReactionEfficiency"))
	{
		return TEXT("元素反应效能");
	}
	if (Target == TEXT("EchoEfficiency"))
	{
		return TEXT("回响效能");
	}
	if (Target == TEXT("FreeShopRefresh"))
	{
		return TEXT("免费商店刷新");
	}
	if (Target == TEXT("CoreCollection"))
	{
		return TEXT("暴击率、暴击效果与元素反应效能");
	}
	if (Target == TEXT("TimeShards"))
	{
		return TEXT("时间碎片");
	}
	if (Target == TEXT("PlayerDamage"))
	{
		return TEXT("玩家伤害增幅");
	}
	if (Target == TEXT("EchoDamage"))
	{
		return TEXT("回响伤害增幅");
	}
	if (Target == TEXT("DamageMultiplier"))
	{
		return TEXT("本关玩家伤害增幅");
	}
	if (Target == TEXT("CritNegateAmplification"))
	{
		return TEXT("暴击时失去全部伤害增幅");
	}
	return TEXT("未命名效果");
}

FText BuildCardOutcomeText(const FReEchoCardDefinition& Card,
                           const FReEchoCardBuildState& State,
                           const FReEchoCardCatalog& Catalog)
{
	TArray<FString> Lines;
	for (const FReEchoCardOutcomeState& Outcome : State.Runtime.ResolvedOutcomes)
	{
		if (Outcome.CardId != Card.Id)
		{
			continue;
		}
		switch (Outcome.Kind)
		{
			case EReEchoCardOutcomeKind::PendingEncounter:
			{
				const int32 Progress =
				    Outcome.PrimaryTarget == TEXT("PhysicalAttack")
				        ? State.Runtime.HuntKillCount
				        : (Outcome.PrimaryTarget == TEXT("ElementalAttack") ? State.Runtime.ReactionCount : 0);
				Lines.Add(FString::Printf(TEXT("等待第%d关结算（当前进度：%d）"), Outcome.EncounterIndex, Progress));
				break;
			}
			case EReEchoCardOutcomeKind::StatTrade:
				Lines.Add(FString::Printf(TEXT("%s %+.0f%%，%s %+.0f%%"),
				                          *GetOutcomeTargetLabel(Outcome.PrimaryTarget),
				                          Outcome.PrimaryValue * 100.0f,
				                          *GetOutcomeTargetLabel(Outcome.SecondaryTarget),
				                          Outcome.SecondaryValue * 100.0f));
				break;
			case EReEchoCardOutcomeKind::GrantedCards:
			{
				TArray<FString> Names;
				for (const FName RelatedCardId : Outcome.RelatedCardIds)
				{
					if (const FReEchoCardDefinition* Related = Catalog.Find(RelatedCardId))
					{
						Names.Add(Related->DisplayName);
					}
				}
				if (!Names.IsEmpty())
				{
					Lines.Add(FString::Printf(TEXT("实际获得：%s"), *FString::Join(Names, TEXT("、"))));
				}
				break;
			}
			case EReEchoCardOutcomeKind::StatGain:
				Lines.Add(FString::Printf(TEXT("实际结算：%s %s"),
				                          *GetOutcomeTargetLabel(Outcome.PrimaryTarget),
				                          *FormatOutcomeValue(Outcome.PrimaryTarget, Outcome.PrimaryValue)));
				break;
			case EReEchoCardOutcomeKind::EconomyPenalty:
				switch (Outcome.EconomyPenalty)
				{
					case EReEchoCardEconomyPenalty::NoShopRefresh:
						Lines.Add(TEXT("实际代价：不能再刷新商店"));
						break;
					case EReEchoCardEconomyPenalty::NoExtraCardPurchase:
						Lines.Add(TEXT("实际代价：不能再购买额外卡牌组"));
						break;
					case EReEchoCardEconomyPenalty::NoEnemyShardDrops:
						Lines.Add(TEXT("实际代价：敌方单位不再掉落时间碎片"));
						break;
					default:
						break;
				}
				break;
			case EReEchoCardOutcomeKind::FreeShopRefreshes:
				Lines.Add(FString::Printf(TEXT("累计获得免费商店刷新：%d次"), FMath::RoundToInt(Outcome.PrimaryValue)));
				break;
			case EReEchoCardOutcomeKind::CumulativeStatGain:
				Lines.Add(FString::Printf(TEXT("累计获得：%s %s"),
				                          *GetOutcomeTargetLabel(Outcome.PrimaryTarget),
				                          *FormatOutcomeValue(Outcome.PrimaryTarget, Outcome.PrimaryValue)));
				if (!Outcome.SecondaryTarget.IsNone())
				{
					Lines.Add(FString::Printf(TEXT("累计获得：%s %s"),
					                          *GetOutcomeTargetLabel(Outcome.SecondaryTarget),
					                          *FormatOutcomeValue(Outcome.SecondaryTarget, Outcome.SecondaryValue)));
				}
				break;
			case EReEchoCardOutcomeKind::RunReset:
				Lines.Add(FString::Printf(TEXT("已移除全部武器符文\n获得%d时间碎片\n获得%d次免费商店刷新"),
				                          FMath::RoundToInt(Outcome.PrimaryValue),
				                          FMath::RoundToInt(Outcome.SecondaryValue)));
				break;
			case EReEchoCardOutcomeKind::FreeShopVisit:
				Lines.Add(FString::Printf(TEXT("第%d关商店商品免费"), Outcome.EncounterIndex));
				break;
			case EReEchoCardOutcomeKind::UnlimitedRefresh:
				Lines.Add(Outcome.PrimaryValue > 0.0f ? TEXT("武器/符文商店可无限刷新，等待下一次购买")
				                                      : TEXT("无限刷新已在购买后消耗"));
				break;
			case EReEchoCardOutcomeKind::Debt:
				Lines.Add(FString::Printf(TEXT("当前欠款：%d时间碎片\n每关利息：10%%（向下取整）"),
				                          FMath::RoundToInt(Outcome.PrimaryValue)));
				break;
			case EReEchoCardOutcomeKind::WeaponMaster:
				Lines.Add(FString::Printf(TEXT("已完成关卡的不同武器：%d种\n累计获得：物攻/元攻 +%d，生命 +%d"),
				                          Outcome.ResolutionCount,
				                          FMath::RoundToInt(Outcome.PrimaryValue),
				                          FMath::RoundToInt(Outcome.SecondaryValue)));
				break;
			case EReEchoCardOutcomeKind::RandomDetails:
				if (Outcome.DetailTargets.IsEmpty())
				{
					Lines.Add(TEXT("本次未获得任何随机效果"));
					break;
				}
				Lines.Add(TEXT("实际效果："));
				for (int32 DetailIndex = 0;
				     DetailIndex < Outcome.DetailTargets.Num() && Outcome.DetailValues.IsValidIndex(DetailIndex);
				     ++DetailIndex)
				{
					const FName Target = Outcome.DetailTargets[DetailIndex];
					const float Value = Outcome.DetailValues[DetailIndex];
					if (Target == TEXT("CritNegateAmplification"))
					{
						Lines.Add(GetOutcomeTargetLabel(Target));
						continue;
					}
					const FString DisplayValue =
					    Target == TEXT("TimeShardMultiplier") || Target == TEXT("NextShardIncomeMultiplier")
					        ? FString::Printf(TEXT("×%.2f"), Value)
					        : FormatOutcomeValue(Target, Value);
					Lines.Add(FString::Printf(TEXT("%s %s"), *GetOutcomeTargetLabel(Target), *DisplayValue));
				}
				break;
			default:
				break;
		}
	}
	return Lines.IsEmpty() ? FText::GetEmpty() : FText::FromString(FString::Join(Lines, TEXT("\n")));
}

FReEchoShopOffer MakeOwnedBuildCardOffer(const FReEchoCardDefinition& Card,
                                         const FReEchoCardBuildState& State,
                                         const FReEchoCardCatalog& Catalog)
{
	FReEchoShopOffer Offer;
	Offer.ItemId = Card.Id;
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.EffectText = FText::FromString(Card.Description);
	Offer.OutcomeText = BuildCardOutcomeText(Card, State, Catalog);
	Offer.Tier = Card.OfferGroup == EasterEggOfferGroup ? 3 : FMath::Clamp(Card.Tier, 1, 3);
	Offer.Price = Offer.Tier * 10;
	Offer.Type = EReEchoShopOfferType::BuildCard;
	Offer.ContentId = Card.Id;
	Offer.IconTexturePath =
	    FString::Printf(TEXT("/Game/ReEcho/Textures/UI/Cards/Icon/T_UI_CardIcon_%s.T_UI_CardIcon_%s"),
	                    *Card.Id.ToString(),
	                    *Card.Id.ToString());
	return Offer;
}

int32 BuildShopOfferSeed(const int32 RunSeed,
                         const FName ContentId,
                         const int32 EncounterIndex,
                         const int32 RefreshSequence)
{
	uint32 Seed = HashCombine(GetTypeHash(RunSeed), GetTypeHash(ContentId));
	Seed = HashCombine(Seed, GetTypeHash(EncounterIndex));
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

TMap<FName, int32> NormalizeRuneCounts(const TArray<FName>& OwnedIds,
                                       const TArray<FReEchoEquippedPartSnapshot>& Equipped,
                                       const TMap<FName, int32>& SavedCounts)
{
	TMap<FName, int32> Counts;
	for (const FName Id : OwnedIds)
	{
		Counts.Add(Id, FMath::Max(1, SavedCounts.FindRef(Id)));
	}
	TMap<FName, int32> EquippedCounts;
	for (const FReEchoEquippedPartSnapshot& Part : Equipped)
	{
		const int32 Minimum = ++EquippedCounts.FindOrAdd(Part.PartId);
		const int32 Held = FMath::Max(Counts.FindRef(Part.PartId), SavedCounts.FindRef(Part.PartId));
		Counts.Add(Part.PartId, FMath::Max(Minimum, Held));
	}
	return Counts;
}

bool TryResolveRuneInventory(const FReEchoCsvDataSnapshot& Snapshot,
                             const FReEchoBuildSnapshot& Build,
                             const TArray<FName>& OwnedIds,
                             const TMap<FName, int32>& Counts,
                             const FName AcquiredId,
                             FReEchoBuildSnapshot& OutBuild,
                             TArray<FName>& OutOwnedIds,
                             TMap<FName, int32>& OutCounts,
                             FString& OutError)
{
	ReEchoRuneInventory::FState Input;
	Input.Counts = NormalizeRuneCounts(OwnedIds, Build.EquippedParts, Counts);
	for (const FReEchoEquippedPartSnapshot& Part : Build.EquippedParts)
	{
		Input.EquippedIds.Add(Part.PartId);
	}
	// Only a purchase supplies AcquiredId. Fill a compatible free slot in the candidate, never
	// displacing an existing rune or auto-equipping unrelated inventory during manual settlement.
	const FReEchoCsvPartRow* AcquiredPart = Snapshot.Parts.Find(AcquiredId);
	const FReEchoCsvWeaponRow* Weapon = Snapshot.FindEnabledWeapon(Build.WeaponId);
	if (AcquiredPart && Weapon && !Input.EquippedIds.Contains(AcquiredId) &&
	    IsPartCompatibleWithWeapon(Snapshot, *AcquiredPart, *Weapon))
	{
		int32 OccupiedSlots = 0;
		for (const FReEchoEquippedPartSnapshot& Equipped : Build.EquippedParts)
		{
			const FReEchoCsvPartRow* EquippedPart = Snapshot.Parts.Find(Equipped.PartId);
			if (EquippedPart && EquippedPart->SlotTypeId == AcquiredPart->SlotTypeId)
			{
				++OccupiedSlots;
			}
		}
		const int32 Capacity = ReEchoWeaponRuntime::GetEffectiveSlotCapacity(
		    Snapshot, Build, Weapon->WeaponTypeId, AcquiredPart->SlotTypeId);
		if (OccupiedSlots < Capacity)
		{
			Input.EquippedIds.Add(AcquiredId);
		}
	}
	ReEchoRuneInventory::FState Resolved;
	FReEchoBuildSnapshot ResolvedBuild;
	if (!ReEchoRuneInventory::TrySettle(Snapshot, Input, AcquiredId, Resolved, OutError) ||
	    !ReEchoWeaponRuntime::TryEquipParts(Snapshot, Build, Resolved.EquippedIds, ResolvedBuild, OutError))
	{
		return false;
	}
	TArray<FName> ResolvedOwnedIds;
	Resolved.Counts.GetKeys(ResolvedOwnedIds);
	ResolvedOwnedIds.Sort(
	    [](const FName A, const FName B)
	    {
		    return A.LexicalLess(B);
	    });
	OutBuild = MoveTemp(ResolvedBuild);
	OutOwnedIds = MoveTemp(ResolvedOwnedIds);
	OutCounts = MoveTemp(Resolved.Counts);
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

int32 MakeNewRunSeed()
{
	const int64 UtcTicks = FDateTime::UtcNow().GetTicks();
	const uint64 HighResolutionTicks = FPlatformTime::Cycles64();
	const uint32 Seed = HashCombine(GetTypeHash(UtcTicks), GetTypeHash(HighResolutionTicks));
	return Seed != 0 ? static_cast<int32>(Seed) : 1;
}

int32 BuildScopedRunSeed(const int32 RunSeed, const uint32 ScopeSalt)
{
	const uint32 Seed = HashCombine(GetTypeHash(RunSeed), ScopeSalt);
	return Seed != 0 ? static_cast<int32>(Seed) : 1;
}

int32 MigrateLegacyTraitOfferSeed(const UReEchoRunSaveGame& SaveGame)
{
	uint32 Seed =
	    HashCombine(GetTypeHash(SaveGame.CurrentBuild.CharacterId), GetTypeHash(SaveGame.CurrentBuild.WeaponId));
	Seed = HashCombine(Seed, GetTypeHash(SaveGame.EncounterIndex));
	Seed = HashCombine(Seed, 0x52454348u); // "RECH": stable migration salt.
	return Seed != 0 ? static_cast<int32>(Seed) : 1;
}

int32 MigrateLegacyRunSeed(const UReEchoRunSaveGame& SaveGame)
{
	const int32 LegacySeed =
	    SaveGame.TraitOfferSeed != 0 ? SaveGame.TraitOfferSeed : MigrateLegacyTraitOfferSeed(SaveGame);
	return BuildScopedRunSeed(LegacySeed, 0x52554E53u); // "RUNS": stable migration salt.
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
	bool bRebuilt = false;
	if (Build.WeaponDomainRevision.IsEmpty() && Build.EquippedParts.IsEmpty())
	{
		OutBuild = BaseBuild;
		bRebuilt = true;
	}
	else
	{
		bRebuilt = ReEchoWeaponRuntime::TryEquipParts(Snapshot, BaseBuild, GetEquippedPartIds(Build), OutBuild, Error);
	}
	if (bRebuilt && OutBuild.RuleFlags.Contains(BossPhase3FinalStatMultiplierFlag))
	{
		UReEchoRunSubsystem::ApplyBossPhase3FinalStatMultiplier(OutBuild.Stats);
	}
	return bRebuilt;
}

FReEchoStoredEchoSummary MakeStoredEchoSummary(const FReEchoRecording& Recording)
{
	FReEchoStoredEchoSummary Summary;
	Summary.RecordingId = Recording.Id;
	Summary.EncounterIndex = Recording.EncounterIndex;
	Summary.Duration = Recording.Duration;
	Summary.MapId = Recording.MapId;
	Summary.CharacterId = Recording.BuildSnapshot.CharacterId;
	Summary.WeaponId = Recording.BuildSnapshot.WeaponId;
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
	bool bHasTimeAnchorRecording = false;
	FReEchoRecording TimeAnchorRecording;
};

bool MigrateBuildState(const int32 SaveVersion, const FReEchoCsvDataSnapshot& Snapshot, FReEchoBuildSnapshot& Build)
{
	if (!Snapshot.FindEnabledWeapon(Build.WeaponId))
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("Save/recording restore rejected because weapon %s is missing or retired."),
		       *Build.WeaponId.ToString());
		return false;
	}
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
	auto ResolveEffectiveCharacterStats = [&Snapshot](const FReEchoCsvCharacterRow& Character)
	{
		FReEchoStatBlock Result = Character.BaseStats;
		ReEchoCharacterAbilityRuntime::ApplyStaticBuildEffects(Snapshot, Character.Id, Result);
		return Result;
	};
	auto ApplyCharacterDelta = [](FReEchoStatBlock& Target, const FReEchoStatBlock& From, const FReEchoStatBlock& To)
	{
		Target.HpMax += To.HpMax - From.HpMax;
		Target.HpPoint = FMath::Min(Target.HpPoint, Target.HpMax);
		Target.PhysicalAttack += To.PhysicalAttack - From.PhysicalAttack;
		Target.ElementalAttack += To.ElementalAttack - From.ElementalAttack;
		Target.AttackSpeed += To.AttackSpeed - From.AttackSpeed;
		Target.MovementSpeed += To.MovementSpeed - From.MovementSpeed;
		Target.CriticalRate += To.CriticalRate - From.CriticalRate;
		Target.CriticalEffect += To.CriticalEffect - From.CriticalEffect;
		Target.EchoEfficiency += To.EchoEfficiency - From.EchoEfficiency;
		Target.ReactionEfficiency += To.ReactionEfficiency - From.ReactionEfficiency;
	};

	// Plan125 retires the old four-card promotion system. BaseCharacterId was written on every run and
	// identifies the player's original selection when an older save already changed CharacterId.
	FString LegacyBaseCharacterId = Build.RuleFlags.FindRef(TEXT("BaseCharacterId"));
	if (LegacyBaseCharacterId.IsEmpty())
	{
		LegacyBaseCharacterId = Build.EquipmentBaseRuleFlags.FindRef(TEXT("BaseCharacterId"));
	}
	const FName CanonicalBaseCharacterId =
	    LegacyBaseCharacterId.IsEmpty() ? NAME_None : Snapshot.ResolveCharacterId(FName(*LegacyBaseCharacterId));
	const FReEchoCsvCharacterRow* CurrentCharacter = Snapshot.FindCharacter(Build.CharacterId);
	const FReEchoCsvCharacterRow* OriginalCharacter = Snapshot.FindCharacter(CanonicalBaseCharacterId);
	if (CurrentCharacter && OriginalCharacter && CurrentCharacter->Id != OriginalCharacter->Id)
	{
		const FReEchoStatBlock CurrentCharacterStats = ResolveEffectiveCharacterStats(*CurrentCharacter);
		const FReEchoStatBlock OriginalCharacterStats = ResolveEffectiveCharacterStats(*OriginalCharacter);
		ApplyCharacterDelta(Build.Stats, CurrentCharacterStats, OriginalCharacterStats);
		if (Build.bHasEquipmentBase)
		{
			ApplyCharacterDelta(Build.EquipmentBaseStats, CurrentCharacterStats, OriginalCharacterStats);
		}
		Build.CharacterId = OriginalCharacter->Id;
		CurrentCharacter = OriginalCharacter;
	}
	if (CurrentCharacter)
	{
		Build.Stats.RoleId = CurrentCharacter->RoleId == TEXT("None") ? NAME_None : CurrentCharacter->RoleId;
		if (Build.bHasEquipmentBase)
		{
			Build.EquipmentBaseStats.RoleId = Build.Stats.RoleId;
		}
	}
	for (const FName LegacyFlag : {FName(TEXT("BaseCharacterId")), FName(TEXT("Promoted")), FName(TEXT("Role"))})
	{
		Build.RuleFlags.Remove(LegacyFlag);
		Build.EquipmentBaseRuleFlags.Remove(LegacyFlag);
	}

	if (!Snapshot.CardCatalog.IsValid())
	{
		return false;
	}
	if (SaveVersion >= 9)
	{
		if (SaveVersion < 16)
		{
			// v15 and earlier stored one directly purchasable card per tier. That shape cannot faithfully
			// represent a paid three-choice pack, so preserve the page key and deterministically rebuild packs.
			Build.CardState.Runtime.ShopCardOfferIds.Reset();
			Build.CardState.Runtime.ShopCardPackStates.Reset();
		}
		if (SaveVersion < 17)
		{
			for (FReEchoShopCardPackRuntimeState& Pack : Build.CardState.Runtime.ShopCardPackStates)
			{
				Pack.SlotRefreshUses.Init(0, Pack.CandidateCardIds.Num());
			}
		}
		if (SaveVersion < 22)
		{
			for (FReEchoShopCardPackRuntimeState& Pack : Build.CardState.Runtime.ShopCardPackStates)
			{
				Pack.OfferHistoryCardIds = Pack.CandidateCardIds;
			}
		}
		if (SaveVersion < 19)
		{
			Build.CardState.Runtime.ResolvedOutcomes.Reset();
			auto AddPendingOutcome = [&](const FName CardId, const FName Target, const int32 EncounterIndex)
			{
				if (!Build.CardState.OwnedCardIds.Contains(CardId) || EncounterIndex == INDEX_NONE)
				{
					return;
				}
				FReEchoCardOutcomeState& Outcome = Build.CardState.Runtime.ResolvedOutcomes.AddDefaulted_GetRef();
				Outcome.CardId = CardId;
				Outcome.Kind = EReEchoCardOutcomeKind::PendingEncounter;
				Outcome.PrimaryTarget = Target;
				Outcome.EncounterIndex = EncounterIndex;
			};
			AddPendingOutcome(
			    TEXT("G_2_05"), TEXT("PhysicalAttack"), Build.CardState.Runtime.HuntTrackingEncounterIndex);
			AddPendingOutcome(
			    TEXT("G_2_06"), TEXT("ElementalAttack"), Build.CardState.Runtime.ReactionTrackingEncounterIndex);
			if (Build.CardState.OwnedCardIds.Contains(TEXT("G_3_17")) &&
			    Build.CardState.Runtime.EconomyPenalty != EReEchoCardEconomyPenalty::None)
			{
				FReEchoCardOutcomeState& Outcome = Build.CardState.Runtime.ResolvedOutcomes.AddDefaulted_GetRef();
				Outcome.CardId = TEXT("G_3_17");
				Outcome.Kind = EReEchoCardOutcomeKind::EconomyPenalty;
				Outcome.EconomyPenalty = Build.CardState.Runtime.EconomyPenalty;
				Outcome.ResolutionCount = 1;
			}
		}
		if (SaveVersion < 20)
		{
			for (FReEchoShopCardPackRuntimeState& Pack : Build.CardState.Runtime.ShopCardPackStates)
			{
				Pack.bPaymentCommitted = Pack.bPurchased;
				Pack.BasePrice = 0; // Lazily derived from the preserved page identity when next projected.
			}
		}
		if (SaveVersion < 21)
		{
			// Plan111 only appends stable cards and typed runtime fields. Existing owned IDs remain valid; adopt the
			// current catalog revision and deterministic zero/default state for mechanics absent from v20 saves.
			Build.CardState.DomainRevision = Snapshot.CardDomainRevision;
			Build.CardState.Runtime.TimeShardDebt = 0;
			Build.CardState.Runtime.bCurseBankDefaulted = false;
			Build.CardState.Runtime.MasteredWeaponIds.Reset();
			Build.CardState.Runtime.ConductAffectedCount = 0;
			Build.CardState.Runtime.ConductAffectedSpawnIndices.Reset();
			Build.CardState.Runtime.ConductPlayerDamageMultiplier = 1.0f;
			Build.CardState.Runtime.EchoTrinityEfficiencyGranted = 0.0f;
		}
		if (SaveVersion < 25)
		{
			// v25 introduces Easter-card state. Older saves cannot own these cards, so the neutral defaults are exact.
			Build.CardState.Runtime.LastEasterStunPulseIndex = 0;
			Build.CardState.Runtime.EasterDamageTaken = 0.0f;
			Build.CardState.Runtime.bEasterDamageCardsGranted = false;
			Build.CardState.Runtime.bHasPreviousEncounterShardIncome = false;
			Build.CardState.Runtime.PreviousEncounterGrossShardIncome = 0;
			Build.CardState.Runtime.CurrentEncounterGrossShardIncome = 0;
			Build.CardState.Runtime.EncounterShardIncomeMultiplier = 1.0f;
		}
		if (!Build.Cards.IsEmpty() || Build.CardState.DomainRevision != Snapshot.CardDomainRevision ||
		    Build.CardState.Runtime.RandomSequence < 0 || Build.CardState.Runtime.PreventedDamageCount < 0 ||
		    Build.CardState.Runtime.HuntKillCount < 0 || Build.CardState.Runtime.ReactionCount < 0 ||
		    Build.CardState.Runtime.EchoKillProgress < 0 || Build.CardState.Runtime.PlayerKillProgress < 0 ||
		    Build.CardState.Runtime.FreeShopRefreshes < 0 || Build.CardState.Runtime.ShopRefreshSequence < 0 ||
		    Build.CardState.Runtime.TimeShardDebt < 0 || Build.CardState.Runtime.ConductAffectedCount < 0 ||
		    Build.CardState.Runtime.ConductPlayerDamageMultiplier < 1.0f ||
		    Build.CardState.Runtime.EchoTrinityEfficiencyGranted < 0.0f ||
		    Build.CardState.Runtime.LastEasterStunPulseIndex < 0 || Build.CardState.Runtime.EasterDamageTaken < 0.0f ||
		    Build.CardState.Runtime.PreviousEncounterGrossShardIncome < 0 ||
		    Build.CardState.Runtime.CurrentEncounterGrossShardIncome < 0 ||
		    !FMath::IsFinite(Build.CardState.Runtime.EncounterShardIncomeMultiplier) ||
		    Build.CardState.Runtime.EncounterShardIncomeMultiplier < 0.0f ||
		    Build.CardState.Runtime.ShopCardOfferEncounterIndex < INDEX_NONE ||
		    Build.CardState.Runtime.ShopCardOfferRefreshSequence < INDEX_NONE ||
		    !Build.CardState.Runtime.ShopCardOfferIds.IsEmpty() ||
		    (!Build.CardState.Runtime.ShopCardPackStates.IsEmpty() &&
		     Build.CardState.Runtime.ShopCardPackStates.Num() != ReEchoShopOfferCountPerGroup) ||
		    static_cast<uint8>(Build.CardState.Runtime.EconomyPenalty) >
		        static_cast<uint8>(EReEchoCardEconomyPenalty::NoEnemyShardDrops) ||
		    (Build.CardState.Runtime.bHasAnchorRecording && !Build.CardState.Runtime.AnchorRecordingId.IsValid()))
		{
			return false;
		}
		TSet<FName> UniqueMasteredWeapons;
		for (const FName WeaponId : Build.CardState.Runtime.MasteredWeaponIds)
		{
			if (!Snapshot.FindEnabledWeapon(WeaponId) || UniqueMasteredWeapons.Contains(WeaponId))
			{
				return false;
			}
			UniqueMasteredWeapons.Add(WeaponId);
		}
		TSet<int32> UniqueConductTargets;
		for (const int32 SpawnIndex : Build.CardState.Runtime.ConductAffectedSpawnIndices)
		{
			if (SpawnIndex <= 0 || UniqueConductTargets.Contains(SpawnIndex))
			{
				return false;
			}
			UniqueConductTargets.Add(SpawnIndex);
		}
		if (Build.CardState.Runtime.ConductAffectedCount != UniqueConductTargets.Num())
		{
			return false;
		}
		for (int32 PackIndex = 0; PackIndex < Build.CardState.Runtime.ShopCardPackStates.Num(); ++PackIndex)
		{
			const FReEchoShopCardPackRuntimeState& Pack = Build.CardState.Runtime.ShopCardPackStates[PackIndex];
			if (Pack.Tier != PackIndex + 1 || Pack.CandidateCardIds.Num() > ReEchoShopOfferCountPerGroup ||
			    Pack.SlotRefreshUses.Num() != Pack.CandidateCardIds.Num() ||
			    Pack.SlotRefreshUses.ContainsByPredicate(
			        [](const int32 Uses)
			        {
				        return Uses < 0;
			        }) ||
			    Pack.BasePrice < 0 || (Pack.bPurchased && !Pack.bPaymentCommitted) ||
			    ((Pack.bPaymentCommitted || Pack.bPurchased) && Pack.CandidateCardIds.IsEmpty()))
			{
				return false;
			}
			TSet<FName> UniquePackCards;
			for (const FName CardId : Pack.CandidateCardIds)
			{
				const FReEchoCardDefinition* Card = Snapshot.CardCatalog->Find(CardId);
				if (CardId.IsNone() || !Card || !Card->bEnabled || !IsCardCompatibleWithPackTier(*Card, Pack.Tier) ||
				    UniquePackCards.Contains(CardId))
				{
					return false;
				}
				UniquePackCards.Add(CardId);
			}
			TSet<FName> UniqueHistoryCards;
			for (const FName CardId : Pack.OfferHistoryCardIds)
			{
				const FReEchoCardDefinition* Card = Snapshot.CardCatalog->Find(CardId);
				if (CardId.IsNone() || !Card || !Card->bEnabled || !IsCardCompatibleWithPackTier(*Card, Pack.Tier) ||
				    UniqueHistoryCards.Contains(CardId))
				{
					return false;
				}
				UniqueHistoryCards.Add(CardId);
			}
			for (const FName CardId : Pack.CandidateCardIds)
			{
				if (!UniqueHistoryCards.Contains(CardId))
				{
					return false;
				}
			}
		}
		TSet<FName> UniqueCards;
		for (const FName CardId : Build.CardState.OwnedCardIds)
		{
			const FReEchoCardDefinition* Card = Snapshot.CardCatalog->Find(CardId);
			if (!Card || !Card->bEnabled ||
			    (Card->OfferGroup != TraitOfferGroup && Card->OfferGroup != EasterEggOfferGroup) ||
			    (Card->StackPolicy == TEXT("Unique") && UniqueCards.Contains(CardId)))
			{
				return false;
			}
			UniqueCards.Add(CardId);
		}
		TSet<FString> UniqueOutcomes;
		for (const FReEchoCardOutcomeState& Outcome : Build.CardState.Runtime.ResolvedOutcomes)
		{
			const FString OutcomeKey = FString::Printf(TEXT("%s|%d|%s|%s"),
			                                           *Outcome.CardId.ToString(),
			                                           static_cast<int32>(Outcome.Kind),
			                                           *Outcome.PrimaryTarget.ToString(),
			                                           *Outcome.SecondaryTarget.ToString());
			if (!Build.CardState.OwnedCardIds.Contains(Outcome.CardId) ||
			    Outcome.Kind == EReEchoCardOutcomeKind::None ||
			    static_cast<uint8>(Outcome.Kind) > static_cast<uint8>(EReEchoCardOutcomeKind::RandomDetails) ||
			    Outcome.ResolutionCount < 0 || Outcome.EncounterIndex < INDEX_NONE ||
			    Outcome.DetailTargets.Num() != Outcome.DetailValues.Num() ||
			    (Outcome.Kind == EReEchoCardOutcomeKind::PendingEncounter && Outcome.EncounterIndex == INDEX_NONE) ||
			    (Outcome.Kind == EReEchoCardOutcomeKind::EconomyPenalty &&
			     Outcome.EconomyPenalty == EReEchoCardEconomyPenalty::None) ||
			    static_cast<uint8>(Outcome.EconomyPenalty) >
			        static_cast<uint8>(EReEchoCardEconomyPenalty::NoEnemyShardDrops) ||
			    UniqueOutcomes.Contains(OutcomeKey))
			{
				return false;
			}
			for (const FName RelatedCardId : Outcome.RelatedCardIds)
			{
				if (!Snapshot.CardCatalog->Find(RelatedCardId))
				{
					return false;
				}
			}
			UniqueOutcomes.Add(OutcomeKey);
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
	OutState.bHasTimeAnchorRecording = true;
	OutState.TimeAnchorRecording = *Anchor;
}

/** Reads the v5 layout and collapses any retired multi-slot payload to its one authoritative anchor. */
void ReadV5EchoStorage(const UReEchoRunSaveGame& SaveGame, FReEchoEchoStorageRestoreState& OutState)
{
	OutState = FReEchoEchoStorageRestoreState{};
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
	if (!SaveGame.CurrentBuild.CardState.Runtime.bHasAnchorRecording)
	{
		return;
	}
	const FGuid AnchorId = SaveGame.CurrentBuild.CardState.Runtime.AnchorRecordingId;
	const FReEchoRecording* Anchor = SaveGame.StoredEchoes.FindByPredicate(
	    [&AnchorId](const FReEchoRecording& Stored)
	    {
		    return Stored.Id == AnchorId;
	    });
	if (Anchor && Anchor->Id.IsValid())
	{
		OutState.bHasTimeAnchorRecording = true;
		OutState.TimeAnchorRecording = *Anchor;
	}
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
	int32 TimeShardDebt = 0;
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
		default:
			return TEXT("Unknown");
	}
}

FReEchoShopPurchaseAuditState CaptureShopPurchaseAuditState(const UReEchoRunSubsystem& RunSubsystem,
                                                            const TSharedPtr<const FReEchoCsvDataSnapshot>& Snapshot)
{
	FReEchoShopPurchaseAuditState State;
	State.TimeShards = RunSubsystem.TimeShards;
	State.TimeShardDebt = RunSubsystem.CurrentBuild.CardState.Runtime.TimeShardDebt;

	const FName EquippedWeaponId = RunSubsystem.CurrentBuild.WeaponId;
	const FReEchoCsvWeaponRow* EquippedWeapon = Snapshot.IsValid() ? Snapshot->FindWeapon(EquippedWeaponId) : nullptr;
	State.EquippedWeapon =
	    DescribeAuditEntry(EquippedWeaponId, EquippedWeapon ? EquippedWeapon->DisplayName : FString());

	for (const FReEchoEquippedPartSnapshot& EquippedPart : RunSubsystem.CurrentBuild.EquippedParts)
	{
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

	TMap<FName, int32> BackpackCounts = NormalizeRuneCounts(
	    RunSubsystem.OwnedPartIds, RunSubsystem.CurrentBuild.EquippedParts, RunSubsystem.RuneAcquisitionCounts);
	for (const FReEchoEquippedPartSnapshot& Equipped : RunSubsystem.CurrentBuild.EquippedParts)
	{
		--BackpackCounts.FindChecked(Equipped.PartId);
	}
	for (const TPair<FName, int32>& Count : BackpackCounts)
	{
		if (Count.Value <= 0)
		{
			continue;
		}
		const FName PartId = Count.Key;
		const FReEchoCsvPartRow* Part = Snapshot.IsValid() ? Snapshot->Parts.Find(PartId) : nullptr;
		State.RuneBackpack.Add(FString::Printf(
		    TEXT("%sx%d"), *DescribeAuditEntry(PartId, Part ? Part->DisplayName : FString()), Count.Value));
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
	    TEXT("[ShopPurchaseAudit] tx=%s phase=%s item=%s shards=%d debt=%d equippedWeapon=[%s] equippedRunes=[%s] "
	         "activeCards=[%s] weaponBackpack=[%s] runeBackpack=[%s] inventory=[%s]"),
	    *TransactionId,
	    Phase,
	    *ItemId.ToString(),
	    State.TimeShards,
	    State.TimeShardDebt,
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

/** Nested commands coalesce notifications; callbacks only see the fully committed build and balance. */
class UReEchoRunSubsystem::FScopedTimeShardBalanceChange
{
public:
	explicit FScopedTimeShardBalanceChange(UReEchoRunSubsystem& InRun, const bool bInEnabled = true)
	    : Run(InRun), bEnabled(bInEnabled)
	{
		if (bEnabled && Run.TimeShardBalanceChangeDepth++ == 0)
		{
			Run.TimeShardBalanceBeforeChange = Run.GetTimeShardBalance();
		}
	}

	~FScopedTimeShardBalanceChange()
	{
		if (bEnabled && --Run.TimeShardBalanceChangeDepth == 0)
		{
			const FReEchoTimeShardBalance Before = Run.TimeShardBalanceBeforeChange;
			const FReEchoTimeShardBalance After = Run.GetTimeShardBalance();
			if (!(Before == After))
			{
				Run.OnTimeShardBalanceChanged.Broadcast(Before, After);
			}
		}
	}

	FScopedTimeShardBalanceChange(const FScopedTimeShardBalanceChange&) = delete;
	FScopedTimeShardBalanceChange& operator=(const FScopedTimeShardBalanceChange&) = delete;

private:
	UReEchoRunSubsystem& Run;
	bool bEnabled;
};

FReEchoTimeShardBalance UReEchoRunSubsystem::GetTimeShardBalance() const
{
	return {TimeShards, FMath::Max(0, CurrentBuild.CardState.Runtime.TimeShardDebt)};
}

void UReEchoRunSubsystem::DebugSetTimeShards(const int64 Amount)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	TimeShards = static_cast<int32>(FMath::Clamp<int64>(Amount, 0, MAX_int32));
}

void UReEchoRunSubsystem::DebugAddTimeShards(const int32 Amount)
{
	DebugSetTimeShards(static_cast<int64>(TimeShards) + Amount);
}

void UReEchoRunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadPlayerProgress();
}

void UReEchoRunSubsystem::LoadPlayerProgress()
{
	bHasViewedStage01To02Cg = false;
	const UReEchoPlayerProgressSaveGame* Progress = Cast<UReEchoPlayerProgressSaveGame>(
	    UGameplayStatics::LoadGameFromSlot(PlayerProgressSaveSlot, RunSaveUserIndex));
	if (!Progress)
	{
		return;
	}
	if (Progress->SaveVersion < 1 || Progress->SaveVersion > UReEchoPlayerProgressSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogReEcho,
		       Warning,
		       TEXT("Player progress save version %d is unsupported; using defaults."),
		       Progress->SaveVersion);
		return;
	}
	bHasViewedStage01To02Cg = Progress->bHasViewedStage01To02Cg;
}

bool UReEchoRunSubsystem::MarkStage01To02CgViewed()
{
	if (bHasViewedStage01To02Cg)
	{
		return true;
	}
	UReEchoPlayerProgressSaveGame* Progress = NewObject<UReEchoPlayerProgressSaveGame>(GetTransientPackage());
	Progress->bHasViewedStage01To02Cg = true;
	if (!UGameplayStatics::SaveGameToSlot(Progress, PlayerProgressSaveSlot, RunSaveUserIndex))
	{
		UE_LOG(LogReEcho, Error, TEXT("[Stage01To02CG] watched state could not be persisted; skip remains locked."));
		return false;
	}
	bHasViewedStage01To02Cg = true;
	UE_LOG(LogReEcho, Display, TEXT("[Stage01To02CG] natural completion persisted account watched state."));
	return true;
}

void UReEchoRunSubsystem::SetPhase(const EReEchoRunPhase NewPhase)
{
	Phase = NewPhase;
	OnPhaseChanged.Broadcast(Phase);
}

void UReEchoRunSubsystem::StartRun(const FName CharacterId, const FName WeaponId)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	EncounterIndex = 0;
	TimeShards = 0;
	RunSeed = MakeNewRunSeed();
	TraitOfferSeed = BuildScopedRunSeed(RunSeed, 0x54524149u);     // "TRAI"
	EnemyShardDropSeed = BuildScopedRunSeed(RunSeed, 0x53485244u); // "SHRD"
	RewardedEnemyShardDropKeys.Reset();
	InventoryItems.Reset();
	OwnedPartIds.Reset();
	OwnedWeaponIds.Reset();
	RuneAcquisitionCounts.Reset();
	PurchasedWeaponPartOfferIds.Reset();
	WeaponPartShopOfferEncounterIndex = INDEX_NONE;
	WeaponPartShopOfferRefreshSequence = INDEX_NONE;
	WeaponPartShopOfferIds.Reset();
	WeaponRuneRefreshSequence = 0;
	WeaponRuneRefreshEncounterIndex = INDEX_NONE;
	WeaponRuneRefreshesUsed = 0;
	bAutomaticAttackMode = true;
	ResetEchoStorage();
	PendingTraitCardIds.Reset();
	PendingTraitCardOfferHistoryIds.Reset();
	PendingTraitCardRefreshUses.Reset();
	PendingTraitCardOfferEncounterIndex = INDEX_NONE;
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
	for (const FReEchoEquippedPartSnapshot& Part : CurrentBuild.EquippedParts)
	{
		OwnedPartIds.AddUnique(Part.PartId);
		++RuneAcquisitionCounts.FindOrAdd(Part.PartId);
	}
	OwnedWeaponIds.Add(CurrentBuild.WeaponId);
	SetPhase(EReEchoRunPhase::Planning);
	ReEchoBuildTrace::LogSnapshot(TEXT("RunStarted"), EncounterIndex, Phase, CurrentBuild);
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
	TArray<FName> PendingOwned = OwnedPartIds;
	// This low-level API also supports initial/programmatic equipment grants; backpack commands
	// validate ownership before calling it. Preserve removed copies when equipment returns to the bag.
	for (const FReEchoEquippedPartSnapshot& Part : CurrentBuild.EquippedParts)
	{
		PendingOwned.AddUnique(Part.PartId);
	}
	for (const FName Id : PartIds)
	{
		PendingOwned.AddUnique(Id);
	}
	TMap<FName, int32> PendingCounts =
	    NormalizeRuneCounts(PendingOwned, CurrentBuild.EquippedParts, RuneAcquisitionCounts);
	if (!TryResolveRuneInventory(*Snapshot,
	                             Candidate,
	                             PendingOwned,
	                             PendingCounts,
	                             NAME_None,
	                             Candidate,
	                             PendingOwned,
	                             PendingCounts,
	                             OutError))
	{
		return false;
	}
	CurrentBuild = MoveTemp(Candidate);
	OwnedPartIds = MoveTemp(PendingOwned);
	RuneAcquisitionCounts = MoveTemp(PendingCounts);
	ReEchoBuildTrace::LogSnapshot(TEXT("RunesEquipped"), EncounterIndex, Phase, CurrentBuild);
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

bool UReEchoRunSubsystem::TryEquipPurchasedPartAt(const FName PartId,
                                                  const int32 TargetOccurrenceIndex,
                                                  FString& OutError)
{
	// Same validation as TryEquipPurchasedPart, but the new part replaces the entry at the requested
	// occurrence of its slot type instead of squeezing the oldest one.
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

	// Collect the equipped entries of the same slot type in order; occurrence N maps to the Nth of them.
	TArray<int32> SameSlotIndices;
	for (int32 Index = 0; Index < DesiredPartIds.Num(); ++Index)
	{
		const FReEchoCsvPartRow* EquippedRow = Snapshot->Parts.Find(DesiredPartIds[Index]);
		if (EquippedRow && EquippedRow->SlotTypeId == Part->SlotTypeId)
		{
			SameSlotIndices.Add(Index);
		}
	}
	if (SameSlotIndices.IsValidIndex(TargetOccurrenceIndex))
	{
		DesiredPartIds[SameSlotIndices[TargetOccurrenceIndex]] = PartId;
	}
	else
	{
		// That occurrence is still empty, so the slot is free: just take it.
		DesiredPartIds.Add(PartId);
	}
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
	TArray<FName> PendingOwned = OwnedPartIds;
	TMap<FName, int32> PendingCounts =
	    NormalizeRuneCounts(OwnedPartIds, CurrentBuild.EquippedParts, RuneAcquisitionCounts);
	for (const FReEchoEquippedPartSnapshot& Part : CurrentBuild.EquippedParts)
	{
		PendingOwned.AddUnique(Part.PartId);
	}
	if (!TryResolveRuneInventory(*Snapshot,
	                             Candidate,
	                             PendingOwned,
	                             PendingCounts,
	                             NAME_None,
	                             Candidate,
	                             PendingOwned,
	                             PendingCounts,
	                             OutError))
	{
		return false;
	}
	CurrentBuild = MoveTemp(Candidate);
	OwnedPartIds = MoveTemp(PendingOwned);
	RuneAcquisitionCounts = MoveTemp(PendingCounts);
	OutError.Reset();
	ReEchoBuildTrace::LogSnapshot(TEXT("WeaponEquipped"),
	                              EncounterIndex,
	                              Phase,
	                              CurrentBuild,
	                              FString::Printf(TEXT("weapon=%s"), *WeaponId.ToString()));
	return true;
}

TArray<FReEchoShopOffer> UReEchoRunSubsystem::GetOwnedBuildCardView() const
{
	TArray<FReEchoShopOffer> OwnedCards;
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return OwnedCards;
	}

	OwnedCards.Reserve(CurrentBuild.CardState.OwnedCardIds.Num());
	for (const FName CardId : CurrentBuild.CardState.OwnedCardIds)
	{
		if (const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId))
		{
			OwnedCards.Add(MakeOwnedBuildCardOffer(*Card, CurrentBuild.CardState, *Snapshot->CardCatalog));
		}
	}
	return OwnedCards;
}

bool UReEchoRunSubsystem::HasOwnedEasterEggCard() const
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return false;
	}

	return CurrentBuild.CardState.OwnedCardIds.ContainsByPredicate(
	    [&Snapshot](const FName CardId)
	    {
		    const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId);
		    return Card && Card->OfferGroup == EasterEggOfferGroup;
	    });
}

namespace
{
int32 GetRuneTierFromPartId(FName PartId)
{
	const FString S = PartId.ToString();
	if (S.EndsWith(TEXT("_III")))
	{
		return 3;
	}
	if (S.EndsWith(TEXT("_II")))
	{
		return 2;
	}
	if (S.EndsWith(TEXT("_I")))
	{
		return 1;
	}
	return 0;
}

FName GetRuneFamilyFromPartId(FName PartId)
{
	const FString S = PartId.ToString();
	if (S.EndsWith(TEXT("_III")))
	{
		return FName(*S.LeftChop(4));
	}
	if (S.EndsWith(TEXT("_II")))
	{
		return FName(*S.LeftChop(3));
	}
	if (S.EndsWith(TEXT("_I")))
	{
		return FName(*S.LeftChop(2));
	}
	return PartId;
}
}

FReEchoWeaponPartShopView UReEchoRunSubsystem::GetWeaponPartShopView()
{
	FReEchoWeaponPartShopView View;
	View.TimeShardDebt = FMath::Max(0, CurrentBuild.CardState.Runtime.TimeShardDebt);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvWeaponRow* Weapon =
	    Snapshot.IsValid() ? Snapshot->FindEnabledWeapon(CurrentBuild.WeaponId) : nullptr;
	if (!Snapshot.IsValid() || !Weapon)
	{
		return View;
	}
	View.WeaponTypeId = Weapon->WeaponTypeId;
	for (const FReEchoShopOffer& CatalogOffer : GetReEchoShopCatalog())
	{
		FReEchoShopOffer ProjectedOffer = CatalogOffer;
		ProjectedOffer.EffectivePrice = GetDiscountedShopPrice(ProjectedOffer.Price);
		ProjectedOffer.bCanPurchase =
		    !InventoryItems.Contains(ProjectedOffer.ItemId) && CanPayShopCost(ProjectedOffer.EffectivePrice);
		View.RunItemOffers.Add(MoveTemp(ProjectedOffer));
	}
	const FReEchoCsvShopRefreshRuleRow* RefreshRule = Snapshot->ShopRefreshRules.Find(TEXT("Default"));
	if (WeaponRuneRefreshEncounterIndex != EncounterIndex)
	{
		WeaponRuneRefreshEncounterIndex = EncounterIndex;
		WeaponRuneRefreshSequence = 0;
		WeaponRuneRefreshesUsed = 0;
	}
	if (RefreshRule)
	{
		View.bUnlimitedShopCredit = GetCardRules().bUnlimitedShopCredit;
		View.bWeaponRuneRefreshUnlimited = CurrentBuild.CardState.Runtime.bUnlimitedWeaponRuneRefresh;
		View.WeaponRuneRefreshesRemaining =
		    FMath::Max(0, RefreshRule->WeaponRuneRefreshLimit - WeaponRuneRefreshesUsed);
		View.WeaponRuneRefreshCost = RefreshRule->WeaponRuneRefreshCost;
		const bool bHasFreeRefresh = CurrentBuild.CardState.Runtime.FreeShopRefreshes > 0;
		const bool bCanUsePaidRefresh = (View.bWeaponRuneRefreshUnlimited || View.WeaponRuneRefreshesRemaining > 0) &&
		                                CanPayShopCost(View.WeaponRuneRefreshCost);
		View.bWeaponRuneRefreshAllowed = !GetCardRules().bDisableShopRefresh && (bHasFreeRefresh || bCanUsePaidRefresh);
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
	const TMap<FName, int32> HeldCounts =
	    NormalizeRuneCounts(OwnedPartIds, CurrentBuild.EquippedParts, RuneAcquisitionCounts);
	for (const auto& Pair : Snapshot->Parts)
	{
		const FReEchoCsvPartRow& Part = Pair.Value;
		if (OwnedPartIds.Contains(Part.PartId) && IsPartCompatibleWithWeapon(*Snapshot, Part, *Weapon))
		{
			FReEchoShopOffer Offer = MakeWeaponPartOffer(Part);
			Offer.BackpackCount = HeldCounts.FindRef(Part.Id) - CurrentBuild.EquippedParts
			                                                        .FilterByPredicate(
			                                                            [&](const FReEchoEquippedPartSnapshot& Equipped)
			                                                            {
				                                                            return Equipped.PartId == Part.Id;
			                                                            })
			                                                        .Num();
			View.OwnedParts.Add(MoveTemp(Offer));
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
	// Rune upgrade context: families that participate in I->II->III synthesis, and each family's highest tier.
	TSet<FName> UpgradeFamilies;
	TMap<FName, FName> FamilyMaxTierPart; // family -> terminal (highest) tier part id
	if (Snapshot->RuneUpgrades.Num() > 0)
	{
		TSet<FName> FromParts;
		for (const TPair<FName, FReEchoCsvRuneUpgradeRow>& Pair : Snapshot->RuneUpgrades)
		{
			UpgradeFamilies.Add(GetRuneFamilyFromPartId(Pair.Key));
			FromParts.Add(Pair.Key);
		}
		for (const TPair<FName, FReEchoCsvRuneUpgradeRow>& Pair : Snapshot->RuneUpgrades)
		{
			// A ToPartId that is never a FromPartId is the highest tier of its family.
			if (!FromParts.Contains(Pair.Value.ToPartId))
			{
				FamilyMaxTierPart.FindOrAdd(GetRuneFamilyFromPartId(Pair.Value.ToPartId)) = Pair.Value.ToPartId;
			}
		}
	}
	const auto IsRuneTierIPart = [&](const FName& PartId) -> bool
	{
		return UpgradeFamilies.Contains(GetRuneFamilyFromPartId(PartId)) && GetRuneTierFromPartId(PartId) == 1;
	};
	const auto OwnsFamilyMaxTier = [&](const FName& PartId) -> bool
	{
		const FName* MaxPart = FamilyMaxTierPart.Find(GetRuneFamilyFromPartId(PartId));
		return MaxPart && (OwnedPartIds.Contains(*MaxPart) || EquippedPartIds.Contains(*MaxPart));
	};
	const auto IsPartExcluded = [&](const FName& PartId) -> bool
	{
		// Tier-I runes keep refreshing in the shop even after the player owns them (repeat-purchase to upgrade).
		if (IsRuneTierIPart(PartId))
		{
			return false;
		}
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
		// Rune-upgrade parts: only the tier-I version is ever sold; higher tiers come from synthesis.
		// Once the player owns the highest tier of a family, stop offering its tier-I version.
		const FName Family = GetRuneFamilyFromPartId(Part.Id);
		if (UpgradeFamilies.Contains(Family))
		{
			if (GetRuneTierFromPartId(Part.Id) != 1)
			{
				continue; // II/III never appear in the shop
			}
			if (OwnsFamilyMaxTier(Part.Id))
			{
				continue; // highest tier owned -> stop refreshing tier I
			}
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

	const int32 WeaponPartRefreshSequence = WeaponRuneRefreshSequence;
	const auto MakePartSlotOffer = [&](const FReEchoCsvPartRow& Part) -> FReEchoWeaponSlotOffer
	{
		FRandomStream PriceRand(BuildShopOfferSeed(RunSeed, Part.Id, EncounterIndex, WeaponPartRefreshSequence));
		FReEchoWeaponSlotOffer Offer;
		Offer.Kind = EReEchoShopOfferKind::Part;
		Offer.PartId = Part.Id;
		Offer.ItemId = Part.Id;
		Offer.ContentId = Part.Id;
		Offer.SlotTypeId = Part.SlotTypeId;
		Offer.WeaponTypeId = Part.WeaponTypeId;
		Offer.DisplayName = FText::FromString(Part.DisplayName);
		Offer.EffectText = FText::FromString(Part.Description);
		Offer.Price = GetShopPriceInRange(*Snapshot, DerivePartPriceCategory(Part), Part.ShopPrice, PriceRand);
		return Offer;
	};
	const auto MakeWeaponSlotOffer = [&](const FReEchoCsvWeaponRow& CandidateWeapon) -> FReEchoWeaponSlotOffer
	{
		FRandomStream PriceRand(
		    BuildShopOfferSeed(RunSeed, CandidateWeapon.Id, EncounterIndex, WeaponPartRefreshSequence));
		FReEchoWeaponSlotOffer Offer;
		Offer.Kind = EReEchoShopOfferKind::Weapon;
		Offer.WeaponId = CandidateWeapon.Id;
		Offer.ItemId = CandidateWeapon.Id;
		Offer.ContentId = CandidateWeapon.Id;
		Offer.SlotTypeId = NAME_None;
		Offer.WeaponTypeId = CandidateWeapon.WeaponTypeId;
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
		// A regenerated page may offer runes again; only the current page tracks what was already bought.
		PurchasedWeaponPartOfferIds.Reset();

		// Weighted pick with fallbacks. Both weapon-rune slots (middle slot 1 and right slot 2) use
		// 90/5/5 per v1.1:
		//   90% current weapon's runes (excluding universal), 5% a whole other weapon,
		//   5% another weapon's runes.
		// Reused as the universal-slot fallback (tracks the same distribution) once every universal
		// rune has already been bought.
		const auto PickWeightedSlotId =
		    [&](FRandomStream& Rand, float CurrentRuneChance, float OtherWeaponChance) -> FName
		{
			const float Roll = Rand.GetFraction();
			if (Roll < CurrentRuneChance && CurrentWeaponRuneCandidates.Num() > 0)
			{
				return PickPart(CurrentWeaponRuneCandidates, Rand)->Id;
			}
			if (Roll < CurrentRuneChance + OtherWeaponChance && OtherWeaponCandidates.Num() > 0)
			{
				return PickWeapon(OtherWeaponCandidates, Rand)->Id;
			}
			if (OtherWeaponRuneCandidates.Num() > 0)
			{
				return PickPart(OtherWeaponRuneCandidates, Rand)->Id;
			}
			if (CurrentWeaponRuneCandidates.Num() > 0)
			{
				return PickPart(CurrentWeaponRuneCandidates, Rand)->Id;
			}
			if (OtherWeaponCandidates.Num() > 0)
			{
				return PickWeapon(OtherWeaponCandidates, Rand)->Id;
			}
			return NAME_None;
		};
		const auto ConsumeChosenId = [&](const FName ChosenId)
		{
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
		};

		// Slot 0: universal rune (deterministic by seed).
		FRandomStream Slot0Rand(BuildShopOfferSeed(RunSeed, View.WeaponId, EncounterIndex, WeaponPartRefreshSequence));
		if (const FReEchoCsvPartRow* Chosen = PickPart(UniversalRuneCandidates, Slot0Rand))
		{
			WeaponPartShopOfferIds[0] = Chosen->Id;
		}
		else if (UniversalRuneCandidates.Num() == 0)
		{
			// Every universal rune is already owned: the slot keeps selling, but switches to the same
			// weighted logic the middle slot uses instead of going empty.
			WeaponPartShopOfferIds[0] = PickWeightedSlotId(Slot0Rand, 0.90f, 0.05f);
			ConsumeChosenId(WeaponPartShopOfferIds[0]);
		}

		// Slots 1 & 2: weighted 90/5/5 pick with fallbacks (v1.1) — both the middle and right
		// weapon-rune slots. Remove each result so one page never duplicates an item.
		FRandomStream SlotRand(
		    BuildShopOfferSeed(RunSeed, TEXT("SHOP_SLOTS"), EncounterIndex, WeaponPartRefreshSequence));
		for (int32 SlotIndex = 1; SlotIndex <= 2; ++SlotIndex)
		{
			const FName ChosenId = PickWeightedSlotId(SlotRand, 0.90f, 0.05f);
			WeaponPartShopOfferIds[SlotIndex] = ChosenId;
			ConsumeChosenId(ChosenId);
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < WeaponPartShopOfferIds.Num(); ++SlotIndex)
	{
		const FName OfferId = WeaponPartShopOfferIds[SlotIndex];
		if (PurchasedWeaponPartOfferIds.Contains(OfferId))
		{
			// Already bought on this page: leave the slot empty so the purchase feels consumed.
			continue;
		}
		if (const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(OfferId))
		{
			// Ownership changes never reroll a cached page. Suppress only the now-ineligible slot.
			if (IsPartExcluded(OfferId) || OwnsFamilyMaxTier(OfferId) ||
			    (UpgradeFamilies.Contains(GetRuneFamilyFromPartId(OfferId)) && !IsRuneTierIPart(OfferId)))
			{
				continue;
			}
			View.SlotOffers[SlotIndex] = MakePartSlotOffer(*Part);
		}
		else if (const FReEchoCsvWeaponRow* CachedWeapon = Snapshot->FindEnabledWeapon(OfferId))
		{
			View.SlotOffers[SlotIndex] = MakeWeaponSlotOffer(*CachedWeapon);
		}
		FReEchoWeaponSlotOffer& ProjectedOffer = View.SlotOffers[SlotIndex];
		if (!ProjectedOffer.ItemId.IsNone())
		{
			ProjectedOffer.EffectivePrice = GetDiscountedShopPrice(ProjectedOffer.Price);
			const bool bOwned = ProjectedOffer.Kind == EReEchoShopOfferKind::Weapon
			                        ? OwnedWeaponIds.Contains(ProjectedOffer.WeaponId)
			                        : OwnedPartIds.Contains(ProjectedOffer.PartId);
			// A tier-I rune stays purchasable while already owned: repeat purchases are the only way to
			// gather the copies that I->II->III synthesis consumes.
			ProjectedOffer.bRepeatPurchasable =
			    ProjectedOffer.Kind == EReEchoShopOfferKind::Part && IsRuneTierIPart(ProjectedOffer.PartId);
			ProjectedOffer.bCanPurchase =
			    (!bOwned || ProjectedOffer.bRepeatPurchasable) && CanPayShopCost(ProjectedOffer.EffectivePrice);
		}
	}

	View.CardPackOffers.SetNum(ReEchoShopOfferCountPerGroup);
	for (int32 PackIndex = 0; PackIndex < View.CardPackOffers.Num(); ++PackIndex)
	{
		FReEchoShopCardPackOffer& CardPack = View.CardPackOffers[PackIndex];
		CardPack.Tier = PackIndex + 1;
		CardPack.DisplayName = FText::FromString(CardPack.Tier == 1   ? TEXT("一级")
		                                         : CardPack.Tier == 2 ? TEXT("二级")
		                                                              : TEXT("三级"));
		CardPack.StatusText = NSLOCTEXT("ReEcho", "ShopCardPackNotOffered", "未投放");
	}

	if (Snapshot->CardCatalog.IsValid())
	{
		FReEchoCardRuntimeState& CardRuntime = CurrentBuild.CardState.Runtime;
		// Card packs no longer share the weapon/rune refresh sequence. Their initial page is stable for the encounter;
		// only TryRefreshShopCardSlot may replace one indexed candidate.
		constexpr int32 RefreshSequence = 0;
		// Build-card shop: fixed [Tier1, Tier2, Tier3] packs. ShopTiers enables matching packs with optional counts
		// (Tier:Count).
		if (const FReEchoCsvShopDropLevelRow* DropLevel = Snapshot->ShopDropLevels.Find(EncounterIndex))
		{
			TMap<int32, int32> ConfiguredCardTierQuantities; // tier -> remaining purchase count
			if (!DropLevel->ShopTiers.IsEmpty())
			{
				TArray<FString> TierTokens;
				DropLevel->ShopTiers.ParseIntoArray(TierTokens, TEXT("|"));
				for (const FString& Tok : TierTokens)
				{
					TArray<FString> Parts;
					Tok.ParseIntoArray(Parts, TEXT(":"));
					const int32 Tier = FCString::Atoi(*(Parts.Num() > 0 ? Parts[0] : Tok));
					const int32 Count = Parts.Num() > 1 ? FCString::Atoi(*Parts[1]) : 1;
					if (Tier >= 1 && Tier <= ReEchoShopOfferCountPerGroup && Count > 0)
					{
						ConfiguredCardTierQuantities.Add(Tier, Count);
					}
				}
			}
			const auto IsCachedCardPageValid = [&]()
			{
				if (CardRuntime.ShopCardPackStates.Num() != ReEchoShopOfferCountPerGroup)
				{
					return false;
				}
				for (int32 PackIndex = 0; PackIndex < ReEchoShopOfferCountPerGroup; ++PackIndex)
				{
					const int32 Tier = PackIndex + 1;
					const FReEchoShopCardPackRuntimeState& Pack = CardRuntime.ShopCardPackStates[PackIndex];
					if (Pack.Tier != Tier || Pack.CandidateCardIds.Num() > ReEchoShopOfferCountPerGroup ||
					    Pack.SlotRefreshUses.Num() != Pack.CandidateCardIds.Num() ||
					    Pack.SlotRefreshUses.ContainsByPredicate(
					        [](const int32 Uses)
					        {
						        return Uses < 0;
					        }) ||
					    Pack.BasePrice < 0 || (Pack.bPurchased && !Pack.bPaymentCommitted))
					{
						return false;
					}
					if (!ConfiguredCardTierQuantities.Contains(Tier))
					{
						if (!Pack.CandidateCardIds.IsEmpty() || Pack.bPaymentCommitted || Pack.bPurchased)
						{
							return false;
						}
						continue;
					}
					if ((Pack.bPaymentCommitted || Pack.bPurchased) && Pack.CandidateCardIds.IsEmpty())
					{
						return false;
					}
					TSet<FName> UniqueCandidates;
					for (const FName CardId : Pack.CandidateCardIds)
					{
						const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId);
						if (CardId.IsNone() || !Card || !Card->bEnabled || !IsCardCompatibleWithPackTier(*Card, Tier) ||
						    UniqueCandidates.Contains(CardId))
						{
							return false;
						}
						UniqueCandidates.Add(CardId);
					}
					TSet<FName> UniqueHistory;
					for (const FName CardId : Pack.OfferHistoryCardIds)
					{
						const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId);
						if (CardId.IsNone() || !Card || !Card->bEnabled || !IsCardCompatibleWithPackTier(*Card, Tier) ||
						    UniqueHistory.Contains(CardId))
						{
							return false;
						}
						UniqueHistory.Add(CardId);
					}
					for (const FName CardId : Pack.CandidateCardIds)
					{
						if (!UniqueHistory.Contains(CardId))
						{
							return false;
						}
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
				CardRuntime.ShopCardOfferIds.Reset();
				CardRuntime.ShopCardPackStates.SetNum(ReEchoShopOfferCountPerGroup);

				for (int32 PackIndex = 0; PackIndex < ReEchoShopOfferCountPerGroup; ++PackIndex)
				{
					const int32 Tier = PackIndex + 1;
					FReEchoShopCardPackRuntimeState& Pack = CardRuntime.ShopCardPackStates[PackIndex];
					Pack.Tier = Tier;
					Pack.CandidateCardIds.Reset();
					Pack.OfferHistoryCardIds.Reset();
					Pack.SlotRefreshUses.Reset();
					Pack.BasePrice = 0;
					Pack.bPaymentCommitted = false;
					Pack.bPurchased = false;
					Pack.RemainingPurchases = 0;
					Pack.ClaimCount = 0;
					if (!ConfiguredCardTierQuantities.Contains(Tier))
					{
						continue;
					}
					Pack.RemainingPurchases = ConfiguredCardTierQuantities[Tier];
					const FName PriceSeedKey(*FString::Printf(TEXT("SHOP_CARD_PACK_TIER_%d"), Tier));
					FRandomStream PriceRand(BuildShopOfferSeed(RunSeed, PriceSeedKey, EncounterIndex, RefreshSequence));
					Pack.BasePrice =
					    GetShopPriceInRange(*Snapshot, *FString::Printf(TEXT("Card_T%d"), Tier), Tier * 10, PriceRand);
				}
			}

			for (int32 PackIndex = 0; PackIndex < ReEchoShopOfferCountPerGroup; ++PackIndex)
			{
				FReEchoShopCardPackOffer& CardPack = View.CardPackOffers[PackIndex];
				const int32 Tier = PackIndex + 1;
				FReEchoShopCardPackRuntimeState& Pack = CardRuntime.ShopCardPackStates[PackIndex];
				if (!Pack.bPaymentCommitted && !Pack.bPurchased)
				{
					// Candidate identity belongs to the payment transaction. Clear legacy/pre-fix unpaid caches so an
					// extra card granted elsewhere in this shop can never shrink a stale three-choice page.
					Pack.CandidateCardIds.Reset();
					Pack.OfferHistoryCardIds.Reset();
					Pack.SlotRefreshUses.Reset();
				}
				CardPack.ItemId = MakeShopCardPackOfferId(EncounterIndex, RefreshSequence, Tier);
				// Mirror the runtime pack's pick count into the view. The choice UI requires exactly
				// GetPaidShopCardPackSelectableCount() picks (read from the runtime pack), while the claim
				// validators compare the submitted ids against this view count. Leaving the view at its
				// default of 1 makes a cadence pack (Sage 3-choose-2) un-claimable: the player picks two,
				// confirms, and every attempt is rejected with "requires an exact number of choices".
				CardPack.SelectableCardCount = FMath::Max(1, Pack.SelectableCardCount);
				if (!ConfiguredCardTierQuantities.Contains(Tier))
				{
					continue;
				}
				if (Pack.BasePrice <= 0)
				{
					const FName PriceSeedKey(*FString::Printf(TEXT("SHOP_CARD_PACK_TIER_%d"), Tier));
					FRandomStream PriceRand(BuildShopOfferSeed(RunSeed, PriceSeedKey, EncounterIndex, RefreshSequence));
					Pack.BasePrice =
					    GetShopPriceInRange(*Snapshot, *FString::Printf(TEXT("Card_T%d"), Tier), Tier * 10, PriceRand);
				}
				CardPack.Price = Pack.BasePrice;
				CardPack.EffectivePrice = GetDiscountedShopPrice(CardPack.Price);
				const bool bHasRemaining = Pack.RemainingPurchases > 0;
				// Easter cards can replace a normal roll, but they do not keep an exhausted tier entrance purchasable.
				const bool bHasEligibleCard =
				    !ReEchoCardRuntime::BuildOfferPool(
				         *Snapshot->CardCatalog, CurrentBuild.CardState, TraitOfferGroup, Tier, EncounterIndex)
				         .IsEmpty();
				CardPack.RemainingPurchases = Pack.RemainingPurchases;
				CardPack.TotalPurchases = ConfiguredCardTierQuantities.Contains(Tier)
				                              ? ConfiguredCardTierQuantities[Tier]
				                              : (bHasRemaining ? 1 : 0);
				CardPack.Status = Pack.bPurchased                     ? EReEchoShopCardPackStatus::Purchased
				                  : Pack.bPaymentCommitted            ? EReEchoShopCardPackStatus::PaidPendingChoice
				                  : bHasRemaining && bHasEligibleCard ? EReEchoShopCardPackStatus::Available
				                                                      : EReEchoShopCardPackStatus::SoldOut;
				if (Pack.bPurchased)
				{
					CardPack.StatusText = NSLOCTEXT("ReEcho", "ShopCardPackPurchased", "已购");
				}
				else if (Pack.bPaymentCommitted)
				{
					CardPack.StatusText = NSLOCTEXT("ReEcho", "ShopCardPackPending", "待选卡");
				}
				else if (bHasRemaining && bHasEligibleCard)
				{
					const int32 TotalQty =
					    ConfiguredCardTierQuantities.Contains(Tier) ? ConfiguredCardTierQuantities[Tier] : 1;
					if (TotalQty > 1)
					{
						CardPack.StatusText =
						    FText::Format(NSLOCTEXT("ReEcho", "ShopCardPackAvailableCount", "可购买 ({0}/{1})"),
						                  FText::AsNumber(TotalQty - Pack.RemainingPurchases + 1),
						                  FText::AsNumber(TotalQty));
					}
					else
					{
						CardPack.StatusText = NSLOCTEXT("ReEcho", "ShopCardPackAvailable", "可购买");
					}
				}
				else
				{
					CardPack.StatusText = NSLOCTEXT("ReEcho", "ShopCardPackSoldOut", "售罄");
				}
				for (int32 CandidateIndex = 0; CandidateIndex < Pack.CandidateCardIds.Num(); ++CandidateIndex)
				{
					const FName CardId = Pack.CandidateCardIds[CandidateIndex];
					const FReEchoCardDefinition* Chosen = Snapshot->CardCatalog->Find(CardId);
					if (!Chosen || !IsCardCompatibleWithPackTier(*Chosen, Tier))
					{
						continue;
					}
					// The cached page remains stable, but an OnGrant effect or another reward can make a
					// tier-two/three candidate owned after page generation. Never project owned unique cards
					// back into either shop or free-choice entrances; do not replace them with a reroll here.
					// Egao Party: easter-egg cards and 样样都通 stack without limit, so an owned copy still
					// projects and can be picked again.
					if (ReEchoCardRuntime::HasCard(CurrentBuild.CardState, CardId) &&
					    (Tier != 1 || Chosen->StackPolicy == TEXT("Unique")) &&
					    !ReEchoCardRuntime::IsRepeatableCard(*Chosen))
					{
						continue;
					}
					const int32 SlotRefreshSequence =
					    Pack.SlotRefreshUses.IsValidIndex(CandidateIndex) ? Pack.SlotRefreshUses[CandidateIndex] : 0;
					FReEchoShopCardChoiceOffer Choice;
					Choice.CardId = Chosen->Id;
					Choice.ItemId = MakeShopCardOfferId(EncounterIndex, RefreshSequence, Chosen->Id);
					Choice.DisplayName = FText::FromString(Chosen->DisplayName);
					Choice.EffectText = FText::FromString(Chosen->Description);
					Choice.Tags = Chosen->Tags;
					Choice.PresentationTier = Chosen->OfferGroup == EasterEggOfferGroup ? 3 : Tier;
					Choice.Tier = Tier;
					Choice.SlotIndex = CandidateIndex;
					Choice.SlotRefreshSequence = SlotRefreshSequence;
					Choice.Price = 0;
					if (RefreshRule)
					{
						Choice.RemainingRefreshes =
						    FMath::Max(0, RefreshRule->CardSlotRefreshLimit - SlotRefreshSequence);
						Choice.RefreshCost = RefreshRule->CardSlotRefreshCost;
						// 只要还有刷新次数、未被全局禁用、且碎片足够，按钮即可用；替换卡在点击时再校验。
						Choice.bCanRefresh = Choice.RemainingRefreshes > 0 && !GetCardRules().bDisableShopRefresh &&
						                     TimeShards >= Choice.RefreshCost;
					}
					CardPack.Choices.Add(MoveTemp(Choice));
				}
				CardPack.bCanPurchase =
				    CardPack.IsAvailable() && CanPurchaseExtraShopCard() && CanPayShopCost(CardPack.EffectivePrice);
			}
		}

		View.OwnedCards = GetOwnedBuildCardView();
	}

	// Backward-compatibility bridge for weapon/rune UI consumers. Card packs are deliberately not flattened:
	// a pack has no single card icon or price and must be opened before a choice can be purchased.
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
		Offer.EffectivePrice = Slot.EffectivePrice;
		Offer.bCanPurchase = Slot.bCanPurchase;
		Offer.bRepeatPurchasable = Slot.bRepeatPurchasable;
		Offer.SlotTypeId = Slot.SlotTypeId;
		Offer.WeaponTypeId = Slot.WeaponTypeId;
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
	ReEchoBuildTrace::LogSnapshot(TEXT("EncounterStarted"), EncounterIndex, Phase, CurrentBuild);
}

void UReEchoRunSubsystem::CompleteEncounter(const FReEchoRecording& Recording,
                                            const bool bPlayerSurvived,
                                            const bool bBossKilled,
                                            const float PlayerCurrentHealth)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
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
	const FReEchoStatBlock PreviousCardHealthStats = CurrentBuild.Stats;
	if (CurrentBuild.CardState.Runtime.BonusShardDropEncounterIndex == EncounterIndex)
	{
		CurrentBuild.CardState.Runtime.BonusShardDropEncounterIndex = INDEX_NONE;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> CardSnapshot = GetRunDataSnapshot();
	FReEchoBuildSnapshot CardEndBuild;
	int32 CardEndShardsGranted = 0;
	int32 CardEndProjectedTimeShards = INDEX_NONE;
	EReEchoHealthAdjustment CardEndHealthAdjustment = EReEchoHealthAdjustment::None;
	const int32 PreCardEndTimeShards = TimeShards;
	if (CardSnapshot.IsValid() && CardSnapshot->CardCatalog.IsValid() &&
	    TryMutateAuthoritativeBuild(
	        *CardSnapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        if (PlayerCurrentHealth >= 0.0f)
		        {
			        BaseBuild.Stats.HpPoint = FMath::Clamp(PlayerCurrentHealth, 0.0f, BaseBuild.Stats.HpMax);
		        }
		        const FReEchoCardEventResult Event =
		            ReEchoCardRuntime::EndEncounter(*CardSnapshot->CardCatalog,
		                                            BaseBuild.CardState,
		                                            BaseBuild.Stats,
		                                            EncounterIndex,
		                                            0,
		                                            TimeShards,
		                                            RunSeed,
		                                            BaseBuild.CardState.Runtime.CurrentEncounterGrossShardIncome);
		        BaseBuild.CardState = Event.CardState;
		        BaseBuild.Stats = Event.Stats;
		        CardEndShardsGranted = Event.TimeShardsGranted;
		        CardEndProjectedTimeShards = Event.ProjectedTimeShards;
		        CardEndHealthAdjustment = Event.HealthAdjustment;
		        return true;
	        },
	        CardEndBuild))
	{
		CurrentBuild = CardEndBuild;
		if (CardEndProjectedTimeShards != INDEX_NONE)
		{
			// The Easter balance swing defines an exact post-multiplier balance. Do not run its delta through
			// G_4_8 a second time; debt repayment still remains authoritative.
			ApplyProjectedCardCurrency(PreCardEndTimeShards, CardEndProjectedTimeShards, false);
		}
		GrantTimeShards(CardEndShardsGranted);
	}
	const FReEchoCardRuleSnapshot EndRules = GetCardRules();
	if (EndRules.bWeaponMaster && !CurrentBuild.WeaponId.IsNone() &&
	    !CurrentBuild.CardState.Runtime.MasteredWeaponIds.Contains(CurrentBuild.WeaponId))
	{
		CurrentBuild.CardState.Runtime.MasteredWeaponIds.Add(CurrentBuild.WeaponId);
		CurrentBuild.Stats.PhysicalAttack += 5.0f;
		CurrentBuild.Stats.ElementalAttack += 5.0f;
		CurrentBuild.Stats.HpMax += 10.0f;
		CurrentBuild.Stats.HpPoint += 10.0f;
		if (CurrentBuild.bHasEquipmentBase)
		{
			CurrentBuild.EquipmentBaseStats.PhysicalAttack += 5.0f;
			CurrentBuild.EquipmentBaseStats.ElementalAttack += 5.0f;
			CurrentBuild.EquipmentBaseStats.HpMax += 10.0f;
			CurrentBuild.EquipmentBaseStats.HpPoint += 10.0f;
		}
		RefreshWeaponMasterOutcome();
	}
	CommitCardHealthAdjustment(PreviousCardHealthStats, CardEndHealthAdjustment);
	if (CurrentBuild.CardState.Runtime.TimeShardDebt > 0)
	{
		const int32 Interest = static_cast<int32>(
		    FMath::Min<int64>(static_cast<int64>(CurrentBuild.CardState.Runtime.TimeShardDebt) / 10,
		                      TNumericLimits<int32>::Max() - CurrentBuild.CardState.Runtime.TimeShardDebt));
		CurrentBuild.CardState.Runtime.TimeShardDebt += Interest;
		RefreshCurseBankOutcome();
		if (EncounterIndex >= GetTotalEncounterCount())
		{
			CurrentBuild.CardState.Runtime.bCurseBankDefaulted = true;
		}
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

bool UReEchoRunSubsystem::SkipPostEncounterCardChoiceForStageTransitionCg()
{
	const bool bCanNormalizePostEncounterPhase =
	    Phase == EReEchoRunPhase::CardChoice || Phase == EReEchoRunPhase::Planning || Phase == EReEchoRunPhase::Shop;
	if (EncounterIndex != 1 || !bCanNormalizePostEncounterPhase)
	{
		return false;
	}
	PendingTraitCardIds.Reset();
	PendingTraitCardOfferHistoryIds.Reset();
	PendingTraitCardRefreshUses.Reset();
	PendingTraitCardOfferEncounterIndex = INDEX_NONE;
	SetPhase(EReEchoRunPhase::Planning);
	UE_LOG(LogReEcho, Display, TEXT("[Stage01To02CG] skipped Encounter 1 free-card phase."));
	return true;
}

TArray<FReEchoTraitCardOffer> UReEchoRunSubsystem::GenerateTraitCardOffers(const int32 RequestedCount)
{
	if (RequestedCount <= 0 || Phase != EReEchoRunPhase::CardChoice)
	{
		return {};
	}
	if (PendingTraitCardOfferEncounterIndex != EncounterIndex || PendingTraitCardIds.Num() != RequestedCount ||
	    PendingTraitCardRefreshUses.Num() != PendingTraitCardIds.Num() ||
	    PendingTraitCardIds.ContainsByPredicate(
	        [&](const FName CardId)
	        {
		        return !PendingTraitCardOfferHistoryIds.Contains(CardId);
	        }))
	{
		PendingTraitCardIds.Reset();
		PendingTraitCardOfferHistoryIds.Reset();
		PendingTraitCardRefreshUses.Reset();
		PendingTraitCardOfferEncounterIndex = INDEX_NONE;
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
	const FReEchoCsvShopRefreshRuleRow* RefreshRule =
	    Snapshot.IsValid() ? Snapshot->ShopRefreshRules.Find(TEXT("Default")) : nullptr;
	auto ProjectPendingOffers = [&]()
	{
		TArray<FReEchoTraitCardOffer> Projected;
		if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
		{
			return Projected;
		}
		for (int32 SlotIndex = 0; SlotIndex < PendingTraitCardIds.Num(); ++SlotIndex)
		{
			const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(PendingTraitCardIds[SlotIndex]);
			if (!Card || !IsCardCompatibleWithPackTier(*Card, FreeTier))
			{
				Projected.Reset();
				return Projected;
			}
			FReEchoTraitCardOffer Offer = MakeTraitOffer(*Card, FreeTier);
			Offer.SlotIndex = SlotIndex;
			if (RefreshRule)
			{
				const int32 Uses = PendingTraitCardRefreshUses[SlotIndex];
				Offer.RemainingRefreshes = FMath::Max(0, RefreshRule->CardSlotRefreshLimit - Uses);
				Offer.RefreshCost = RefreshRule->CardSlotRefreshCost;
				// 只要还有刷新次数、未被全局禁用、且碎片足够，按钮即可用；具体替换卡在点击时再校验，
				// 不再要求“当前已存在未拥有的替换卡”，避免玩家仍有碎片却被错误置灰。
				Offer.bCanRefresh = Uses < RefreshRule->CardSlotRefreshLimit && !GetCardRules().bDisableShopRefresh &&
				                    TimeShards >= Offer.RefreshCost;
			}
			Projected.Add(MoveTemp(Offer));
		}
		return Projected;
	};
	if (!PendingTraitCardIds.IsEmpty())
	{
		const TArray<FReEchoTraitCardOffer> StableOffers = ProjectPendingOffers();
		if (StableOffers.Num() == RequestedCount)
		{
			return StableOffers;
		}
		PendingTraitCardIds.Reset();
		PendingTraitCardOfferHistoryIds.Reset();
		PendingTraitCardRefreshUses.Reset();
		PendingTraitCardOfferEncounterIndex = INDEX_NONE;
	}
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		UE_LOG(
		    LogReEcho, Error, TEXT("Encounter %d cannot build free card offers: catalog unavailable."), EncounterIndex);
		SetPhase(EReEchoRunPhase::Planning);
		return {};
	}
	const int32 MaximumDistinctOffers =
	    ReEchoCardRuntime::BuildOfferPool(
	        *Snapshot->CardCatalog, CurrentBuild.CardState, TraitOfferGroup, FreeTier, EncounterIndex)
	        .Num() +
	    ReEchoCardRuntime::BuildOfferPool(
	        *Snapshot->CardCatalog, CurrentBuild.CardState, EasterEggOfferGroup, INDEX_NONE, EncounterIndex)
	        .Num();
	if (MaximumDistinctOffers < RequestedCount)
	{
		UE_LOG(
		    LogReEcho,
		    Error,
		    TEXT("Encounter %d FreeTier %d requires %d card offers but only %d are eligible; continuing to the shop."),
		    EncounterIndex,
		    FreeTier,
		    RequestedCount,
		    MaximumDistinctOffers);
		SetPhase(EReEchoRunPhase::Planning);
		return {};
	}
	TArray<FReEchoTraitCardOffer> Result;
	Result.Reserve(RequestedCount);
	// Generate one mutually compatible offer set. For Sage's 3-choose-2 cadence, each subsequently selected
	// candidate is simulated as owned so conflicting cards never appear together and every displayed pair is valid.
	FReEchoCardBuildState CandidateState = CurrentBuild.CardState;
	for (int32 OfferIndex = 0; OfferIndex < RequestedCount; ++OfferIndex)
	{
		uint32 Seed =
		    GetTypeHash(BuildTraitOfferSeed(TraitOfferSeed, EncounterIndex, CurrentBuild.CardState.OwnedCardIds));
		Seed = HashCombine(Seed, GetTypeHash(OfferIndex));
		const FName SelectedCardId = SelectCardForOfferSlot(*Snapshot->CardCatalog,
		                                                    CandidateState,
		                                                    FreeTier,
		                                                    EncounterIndex,
		                                                    PendingTraitCardOfferHistoryIds,
		                                                    static_cast<int32>(Seed));
		const FReEchoCardDefinition* Selected = Snapshot->CardCatalog->Find(SelectedCardId);
		if (!Selected)
		{
			UE_LOG(LogReEcho,
			       Error,
			       TEXT("Encounter %d FreeTier %d exhausted while generating slot %d."),
			       EncounterIndex,
			       FreeTier,
			       OfferIndex);
			SetPhase(EReEchoRunPhase::Planning);
			return {};
		}
		Result.Add(MakeTraitOffer(*Selected, FreeTier));
		CandidateState.OwnedCardIds.Add(SelectedCardId);
		PendingTraitCardOfferHistoryIds.Add(SelectedCardId);
	}

	for (const FReEchoTraitCardOffer& Offer : Result)
	{
		PendingTraitCardIds.Add(Offer.CardId);
		PendingTraitCardOfferHistoryIds.AddUnique(Offer.CardId);
	}
	PendingTraitCardRefreshUses.Init(0, PendingTraitCardIds.Num());
	PendingTraitCardOfferEncounterIndex = EncounterIndex;
	// Cadence abilities (for example the Sage) can let this pack hand out more than one card, turning the
	// usual 3-choose-1 into 3-choose-2. Resolved here so the choice screen knows how many picks to require.
	PendingTraitCardSelectableCount = FMath::Max(1, ResolvePackSelectableCardCount(FreeTier));
	PendingTraitCardPicksRemaining = PendingTraitCardSelectableCount;
	return ProjectPendingOffers();
}

bool UReEchoRunSubsystem::TryRefreshTraitCardSlot(const int32 SlotIndex, FString& OutError)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvShopRefreshRuleRow* RefreshRule =
	    Snapshot.IsValid() ? Snapshot->ShopRefreshRules.Find(TEXT("Default")) : nullptr;
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid() || !RefreshRule)
	{
		OutError = TEXT("Post-encounter card refresh data is unavailable");
		return false;
	}
	if (Phase != EReEchoRunPhase::CardChoice || GetCardRules().bDisableShopRefresh)
	{
		OutError = TEXT("The current run state disables post-encounter card refreshes");
		return false;
	}
	const TArray<FReEchoTraitCardOffer> CurrentOffers = GenerateTraitCardOffers(3);
	if (!CurrentOffers.IsValidIndex(SlotIndex) || !PendingTraitCardRefreshUses.IsValidIndex(SlotIndex))
	{
		OutError = TEXT("The requested post-encounter card slot is unavailable");
		return false;
	}
	const int32 CurrentUses = PendingTraitCardRefreshUses[SlotIndex];
	if (CurrentUses >= RefreshRule->CardSlotRefreshLimit)
	{
		OutError = TEXT("This post-encounter card slot has already used its refresh opportunity");
		return false;
	}
	const int32 FreeTier = ResolveConfiguredFreeTraitTier();
	if (TimeShards < RefreshRule->CardSlotRefreshCost)
	{
		OutError = FString::Printf(TEXT("Time shards %d are below the card-slot refresh cost %d"),
		                           TimeShards,
		                           RefreshRule->CardSlotRefreshCost);
		return false;
	}

	const int32 NextSequence = CurrentUses + 1;
	uint32 Seed = HashCombine(GetTypeHash(TraitOfferSeed), GetTypeHash(EncounterIndex));
	Seed = HashCombine(Seed, GetTypeHash(SlotIndex));
	Seed = HashCombine(Seed, GetTypeHash(NextSequence));
	const FName ReplacementCardId = SelectCardForOfferSlot(*Snapshot->CardCatalog,
	                                                       CurrentBuild.CardState,
	                                                       FreeTier,
	                                                       EncounterIndex,
	                                                       PendingTraitCardOfferHistoryIds,
	                                                       static_cast<int32>(Seed),
	                                                       false);
	if (ReplacementCardId.IsNone())
	{
		OutError = TEXT("No legal unowned same-tier or Easter post-encounter replacement remains");
		return false;
	}
	const FName PreviousCardId = PendingTraitCardIds[SlotIndex];
	TimeShards -= RefreshRule->CardSlotRefreshCost;
	PendingTraitCardIds[SlotIndex] = ReplacementCardId;
	PendingTraitCardOfferHistoryIds.AddUnique(ReplacementCardId);
	PendingTraitCardRefreshUses[SlotIndex] = NextSequence;
	OutError.Reset();
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[FreeCardRefresh] encounter=%d tier=%d slot=%d previous=%s replacement=%s uses=%d cost=%d"),
	       EncounterIndex,
	       FreeTier,
	       SlotIndex,
	       *PreviousCardId.ToString(),
	       *ReplacementCardId.ToString(),
	       NextSequence,
	       RefreshRule->CardSlotRefreshCost);
	return true;
}

bool UReEchoRunSubsystem::ApplyTraitCard(const FName CardId)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	// A cadence pack must be resolved as one exact multi-card transaction; a single-card callback may not
	// silently close an outstanding two-card choice.
	if (Phase != EReEchoRunPhase::CardChoice || PendingTraitCardPicksRemaining != 1 ||
	    !PendingTraitCardIds.Contains(CardId))
	{
		return false;
	}

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid() || !Snapshot->CardCatalog->Find(CardId))
	{
		return false;
	}

	int32 PendingTimeShards = TimeShards;
	bool bPendingClearWeaponRunes = false;
	EReEchoHealthAdjustment PendingHealthAdjustment = EReEchoHealthAdjustment::None;
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
		        PendingHealthAdjustment = Grant.HealthAdjustment;
		        PendingTimeShards = Grant.TimeShards;
		        bPendingClearWeaponRunes |= Grant.bClearWeaponRunes;
		        if (Grant.bClearWeaponRunes)
		        {
			        BaseBuild.EquippedParts.Reset();
		        }
		        // The post-encounter card pack is tiered the same way shop packs are (shop_drop_levels FreeTier),
		        // so feed its tier into the cadence counter; INDEX_NONE means no pack was dropped this encounter.
		        // A pack that owes several picks (cadence ability) is counted once, when the last pick lands.
		        if (PendingTraitCardPicksRemaining <= 1)
		        {
			        RecordNormalTraitGroupSelection(*Snapshot, BaseBuild, ResolveConfiguredFreeTraitTier());
		        }
		        return true;
	        },
	        PendingBuild))
	{
		return false;
	}
	const int32 PreviousTimeShards = TimeShards;
	CurrentBuild = PendingBuild;
	ApplyProjectedCardCurrency(PreviousTimeShards, PendingTimeShards);
	if (bPendingClearWeaponRunes)
	{
		OwnedPartIds.Reset();
		RuneAcquisitionCounts.Reset();
	}
	PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, ReevaluateCoreCollectionCard());
	// NOTE: the pack is closed unconditionally for now. Keeping it open for a second pick requires the
	// choice screen to be reopened by the UI flow; without that the run would sit in the card-choice phase
	// with no interactive screen, which is exactly the mid-combat pop-up bug. Enable together with the
	// multi-select choice screen.
	PendingTraitCardPicksRemaining = 1;
	PendingTraitCardIds.Reset();
	PendingTraitCardOfferHistoryIds.Reset();
	PendingTraitCardRefreshUses.Reset();
	PendingTraitCardOfferEncounterIndex = INDEX_NONE;
	// Always return to planning: cadence bonuses are resolved inside the pack, never as a deferred choice.
	SetPhase(EReEchoRunPhase::Planning);
	ReEchoBuildTrace::LogSnapshot(TEXT("FreeCardGranted"),
	                              EncounterIndex,
	                              Phase,
	                              CurrentBuild,
	                              FString::Printf(TEXT("card=%s"), *CardId.ToString()));
	CommitCardHealthAdjustment(PreviousStats, PendingHealthAdjustment);
	return true;
}

bool UReEchoRunSubsystem::ApplyTraitCards(const TArray<FName>& CardIds)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	// Cadence abilities (the Sage) can let a pack hand out two cards. Both are granted inside one
	// authoritative mutation so the pack closes once, the phase returns to planning once, and the run can
	// never be stranded in the card-choice phase without a screen.
	if (Phase != EReEchoRunPhase::CardChoice || CardIds.Num() != PendingTraitCardPicksRemaining)
	{
		return false;
	}
	TSet<FName> UniqueCardIds;
	for (const FName& CardId : CardIds)
	{
		if (CardId.IsNone() || UniqueCardIds.Contains(CardId))
		{
			return false;
		}
		UniqueCardIds.Add(CardId);
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return false;
	}
	for (const FName& CardId : CardIds)
	{
		if (!PendingTraitCardIds.Contains(CardId) || !Snapshot->CardCatalog->Find(CardId))
		{
			return false;
		}
	}

	int32 PendingTimeShards = TimeShards;
	bool bPendingClearWeaponRunes = false;
	EReEchoHealthAdjustment PendingHealthAdjustment = EReEchoHealthAdjustment::None;
	FReEchoBuildSnapshot PendingBuild;
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        for (const FName& CardId : CardIds)
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
			        PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, Grant.HealthAdjustment);
			        PendingTimeShards = Grant.TimeShards;
			        if (Grant.bClearWeaponRunes)
			        {
				        bPendingClearWeaponRunes = true;
				        BaseBuild.EquippedParts.Reset();
			        }
		        }
		        // One pack, one cadence step, regardless of how many cards it yields.
		        RecordNormalTraitGroupSelection(*Snapshot, BaseBuild, ResolveConfiguredFreeTraitTier());
		        return true;
	        },
	        PendingBuild))
	{
		return false;
	}
	const int32 PreviousTimeShards = TimeShards;
	CurrentBuild = PendingBuild;
	ApplyProjectedCardCurrency(PreviousTimeShards, PendingTimeShards);
	if (bPendingClearWeaponRunes)
	{
		OwnedPartIds.Reset();
		RuneAcquisitionCounts.Reset();
	}
	PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, ReevaluateCoreCollectionCard());
	PendingTraitCardPicksRemaining = 1;
	PendingTraitCardIds.Reset();
	PendingTraitCardOfferHistoryIds.Reset();
	PendingTraitCardRefreshUses.Reset();
	PendingTraitCardOfferEncounterIndex = INDEX_NONE;
	SetPhase(EReEchoRunPhase::Planning);
	ReEchoBuildTrace::LogSnapshot(TEXT("FreeCardsGranted"),
	                              EncounterIndex,
	                              Phase,
	                              CurrentBuild,
	                              FString::Printf(TEXT("count=%d"), CardIds.Num()));
	CommitCardHealthAdjustment(PreviousStats, PendingHealthAdjustment);
	return true;
}

void UReEchoRunSubsystem::ResetPendingTraitCardChoice()
{
	PendingTraitCardIds.Reset();
	PendingTraitCardOfferHistoryIds.Reset();
	PendingTraitCardRefreshUses.Reset();
	PendingTraitCardOfferEncounterIndex = INDEX_NONE;
}

bool UReEchoRunSubsystem::DebugGrantCard(const FName CardId, const bool bSuppressEgaoBonusGrant)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
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

	EReEchoHealthAdjustment PendingHealthAdjustment = EReEchoHealthAdjustment::None;
	FReEchoBuildSnapshot PendingBuild;
	int32 PendingTimeShards = TimeShards;
	bool bPendingClearWeaponRunes = false;
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
		        Input.bSuppressEgaoBonusGrant = bSuppressEgaoBonusGrant;
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
		        PendingTimeShards = Grant.TimeShards;
		        PendingHealthAdjustment = Grant.HealthAdjustment;
		        bPendingClearWeaponRunes |= Grant.bClearWeaponRunes;
		        if (Grant.bClearWeaponRunes)
		        {
			        BaseBuild.EquippedParts.Reset();
		        }
		        UE_LOG(LogReEcho,
		               Warning,
		               TEXT("[DebugGrantCard] identity preserved: Character=%s Cards=%d"),
		               *BaseBuild.CharacterId.ToString(),
		               BaseBuild.CardState.OwnedCardIds.Num());
		        return true;
	        },
	        PendingBuild))
	{
		UE_LOG(LogReEcho, Warning, TEXT("[DebugGrantCard] abort: TryMutateAuthoritativeBuild rejected"));
		return false;
	}
	const int32 PreviousTimeShards = TimeShards;
	CurrentBuild = PendingBuild;
	ApplyProjectedCardCurrency(PreviousTimeShards, PendingTimeShards);
	if (bPendingClearWeaponRunes)
	{
		OwnedPartIds.Reset();
		RuneAcquisitionCounts.Reset();
	}
	PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, ReevaluateCoreCollectionCard());
	UE_LOG(LogReEcho,
	       Warning,
	       TEXT("[DebugGrantCard] done: CardId=%s finalCards=%d"),
	       *CardId.ToString(),
	       CurrentBuild.CardState.OwnedCardIds.Num());
	ReEchoBuildTrace::LogSnapshot(TEXT("DebugCardGranted"),
	                              EncounterIndex,
	                              Phase,
	                              CurrentBuild,
	                              FString::Printf(TEXT("card=%s"), *CardId.ToString()));
	CommitCardHealthAdjustment(PreviousStats, PendingHealthAdjustment);
	return true;
}

FReEchoCardRuleSnapshot UReEchoRunSubsystem::GetCardRules() const
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	return Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()
	           ? ReEchoCardRuntime::CompileRules(*Snapshot->CardCatalog, CurrentBuild.CardState)
	           : FReEchoCardRuleSnapshot{};
}

bool UReEchoRunSubsystem::ActivateBossPhase3FinalStatMultiplier()
{
	if (CurrentBuild.RuleFlags.Contains(BossPhase3FinalStatMultiplierFlag))
	{
		return false;
	}
	if (!CurrentBuild.bHasEquipmentBase)
	{
		CurrentBuild.EquipmentBaseStats = CurrentBuild.Stats;
		CurrentBuild.EquipmentBaseRuleFlags = CurrentBuild.RuleFlags;
		CurrentBuild.bHasEquipmentBase = true;
	}
	CurrentBuild.EquipmentBaseRuleFlags.Add(BossPhase3FinalStatMultiplierFlag, TEXT("2"));
	CurrentBuild.RuleFlags.Add(BossPhase3FinalStatMultiplierFlag, TEXT("2"));
	ApplyBossPhase3FinalStatMultiplier(CurrentBuild.Stats);
	return true;
}

void UReEchoRunSubsystem::ApplyBossPhase3FinalStatMultiplier(FReEchoStatBlock& Stats)
{
	Stats.HpPoint *= 2.0f;
	Stats.HpMax *= 2.0f;
	Stats.PhysicalAttack *= 2.0f;
	Stats.ElementalAttack *= 2.0f;
	Stats.Block *= 2;
	Stats.AttackSpeed *= 2.0f;
	Stats.MovementSpeed *= 2.0f;
	Stats.CriticalRate *= 2.0f;
	Stats.CriticalEffect *= 2.0f;
	Stats.EchoEfficiency *= 2.0f;
	Stats.ReactionEfficiency *= 2.0f;
	Stats.ProjectileCount *= 2;
	Stats.WeaponSize *= 2.0f;
	Stats.EchoCount *= 2;
	Stats.ShopDiscount *= 2.0f;
	Stats.CharacterSize *= 2.0f;
	Stats.Concentration *= 2.0f;
	Stats.PathAffinity *= 2.0f;
}

int32 UReEchoRunSubsystem::BuildCardEffectRandomSeed(const FName ContextId, const int32 Sequence) const
{
	return BuildShopOfferSeed(RunSeed, ContextId, EncounterIndex, Sequence);
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
                                                const float NearestEchoDistanceCm,
                                                const bool bHasLivingEcho,
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
	Input.NearestEchoDistanceCm = NearestEchoDistanceCm;
	Input.bHasLivingEcho = bHasLivingEcho;
	Input.bCritical = Intent.bCritical;
	Input.bTargetHasElement = bTargetHasElement;
	Input.DamageSource = Intent.DamageSource;
	Input.Element = Intent.Element;
	if (const IReEchoCombatTarget* TargetRules = Cast<IReEchoCombatTarget>(Intent.Target))
	{
		Input.TargetDefinitionId = TargetRules->GetCombatTargetDefinitionId();
	}
	if (const UReEchoCombatantComponent* TargetCombatant =
	        Intent.Target ? Intent.Target->FindComponentByClass<UReEchoCombatantComponent>() : nullptr)
	{
		Input.bPreviousPlayerEchoSourceKnown = TargetCombatant->HasLastPlayerEchoDamageSource();
		Input.PreviousPlayerEchoSource = TargetCombatant->GetLastPlayerEchoDamageSource();
	}
	Input.TimeShards = TimeShards;
	Input.RandomSeed = BuildCardEffectRandomSeed(TEXT("CARD_OUTGOING_HIT"), Intent.Attack.Sequence);
	const FReEchoCardOutgoingHitResult Result =
	    ReEchoCardRuntime::ModifyOutgoingHit(*Snapshot->CardCatalog, CurrentBuild.CardState, Input);
	CurrentBuild.CardState = Result.CardState;
	TimeShards = Result.TimeShards;
	Intent.RawDamage = Result.RawDamage;
	Intent.bCritical = Result.bCritical;
	Intent.Element = Result.Element;
	for (const FName StatusId : Result.PreDamageStatusIds)
	{
		Intent.PreDamageStatusIds.AddUnique(StatusId);
	}
}

float UReEchoRunSubsystem::ModifyCardIncomingHit(const float RawDamage, const FName AttackerDefinitionId)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return RawDamage;
	}
	const FReEchoCardIncomingHitResult Result = ReEchoCardRuntime::ModifyIncomingHit(
	    *Snapshot->CardCatalog, CurrentBuild.CardState, RawDamage, TimeShards, AttackerDefinitionId);
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

void UReEchoRunSubsystem::NotifyCardReactionAffectedTargets(const FName ReactionId,
                                                            const TArray<int32>& AffectedSpawnIndices)
{
	if (ReactionId != TEXT("Y_ER_L_W") || AffectedSpawnIndices.IsEmpty())
	{
		return;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid() ||
	    !CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_2_33")))
	{
		return;
	}
	const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(TEXT("G_2_33"));
	if (!Card)
	{
		return;
	}
	float BonusPerTarget = 0.0f;
	for (const FReEchoCardEffectDefinition& Effect : Card->Effects)
	{
		if (Effect.BehaviorId == TEXT("Card.ConductDamageGrowth"))
		{
			BonusPerTarget += Effect.Value;
		}
	}
	if (BonusPerTarget <= 0.0f)
	{
		return;
	}
	FReEchoCardRuntimeState& Runtime = CurrentBuild.CardState.Runtime;
	for (const int32 SpawnIndex : AffectedSpawnIndices)
	{
		if (SpawnIndex > 0)
		{
			Runtime.ConductAffectedSpawnIndices.AddUnique(SpawnIndex);
		}
	}
	Runtime.ConductAffectedCount = Runtime.ConductAffectedSpawnIndices.Num();
	Runtime.ConductPlayerDamageMultiplier = 1.0f + Runtime.ConductAffectedCount * BonusPerTarget;
	FReEchoCardOutcomeState* Outcome = Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Candidate)
	    {
		    return Candidate.CardId == TEXT("G_2_33");
	    });
	if (!Outcome)
	{
		Outcome = &Runtime.ResolvedOutcomes.AddDefaulted_GetRef();
		Outcome->CardId = TEXT("G_2_33");
	}
	Outcome->Kind = EReEchoCardOutcomeKind::CumulativeStatGain;
	Outcome->PrimaryTarget = TEXT("DamageMultiplier");
	Outcome->PrimaryValue = Runtime.ConductAffectedCount * BonusPerTarget;
	Outcome->ResolutionCount = Runtime.ConductAffectedCount;
}

float UReEchoRunSubsystem::GetCardReactionDamageMultiplier(const FName ReactionId) const
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	return Snapshot.IsValid() && Snapshot->CardCatalog.IsValid()
	           ? ReEchoCardRuntime::GetReactionDamageMultiplier(
	                 *Snapshot->CardCatalog, CurrentBuild.CardState, ReactionId)
	           : 1.0f;
}

void UReEchoRunSubsystem::NotifyCardKill(const bool bKilledByEcho, const FName TargetDefinitionId)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	FReEchoBuildSnapshot PendingBuild;
	int32 ShardsGranted = 0;
	EReEchoHealthAdjustment HealthAdjustment = EReEchoHealthAdjustment::None;
	if (Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() &&
	    TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardEventResult Event = ReEchoCardRuntime::OnKillResolved(
		            *Snapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats, bKilledByEcho, TargetDefinitionId);
		        BaseBuild.CardState = Event.CardState;
		        BaseBuild.Stats = Event.Stats;
		        ShardsGranted = Event.TimeShardsGranted;
		        HealthAdjustment = Event.HealthAdjustment;
		        return true;
	        },
	        PendingBuild))
	{
		CurrentBuild = PendingBuild;
		GrantTimeShards(ShardsGranted);
		CommitCardHealthAdjustment(PreviousStats, HealthAdjustment);
	}
}

float UReEchoRunSubsystem::NotifyCardDamageResolved(const float RawDamage,
                                                    const float AppliedDamage,
                                                    const bool bDealtByEcho)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	FReEchoBuildSnapshot PendingBuild;
	float Healing = 0.0f;
	if (Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() &&
	    TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardEventResult Event = ReEchoCardRuntime::OnDamageResolved(*Snapshot->CardCatalog,
		                                                                                 BaseBuild.CardState,
		                                                                                 BaseBuild.Stats,
		                                                                                 RawDamage,
		                                                                                 AppliedDamage,
		                                                                                 bDealtByEcho);
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

void UReEchoRunSubsystem::NotifyCardPlayerDamageReceived(const float AppliedDamage)
{
	if (AppliedDamage <= 0.0f)
	{
		return;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return;
	}
	const int32 PreviousTimeShards = TimeShards;
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	int32 ProjectedTimeShards = TimeShards;
	bool bClearWeaponRunes = false;
	EReEchoHealthAdjustment HealthAdjustment = EReEchoHealthAdjustment::None;
	FReEchoBuildSnapshot PendingBuild;
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardGrantResult Grant = ReEchoCardRuntime::OnPlayerDamageReceived(*Snapshot->CardCatalog,
		                                                                                       BaseBuild.CardState,
		                                                                                       BaseBuild.Stats,
		                                                                                       TimeShards,
		                                                                                       AppliedDamage,
		                                                                                       EncounterIndex,
		                                                                                       RunSeed);
		        if (!Grant.bSucceeded)
		        {
			        return false;
		        }
		        BaseBuild.CardState = Grant.CardState;
		        BaseBuild.Stats = Grant.Stats;
		        ProjectedTimeShards = Grant.TimeShards;
		        bClearWeaponRunes = Grant.bClearWeaponRunes;
		        HealthAdjustment = Grant.HealthAdjustment;
		        if (bClearWeaponRunes)
		        {
			        BaseBuild.EquippedParts.Reset();
		        }
		        return true;
	        },
	        PendingBuild))
	{
		return;
	}
	CurrentBuild = MoveTemp(PendingBuild);
	ApplyProjectedCardCurrency(PreviousTimeShards, ProjectedTimeShards);
	if (bClearWeaponRunes)
	{
		OwnedPartIds.Reset();
		RuneAcquisitionCounts.Reset();
	}
	CommitCardHealthAdjustment(PreviousStats, HealthAdjustment);
}

float UReEchoRunSubsystem::NotifyCardNegativeStatusApplied(const FName StatusId, const bool bAppliedByEcho)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	FReEchoBuildSnapshot PendingBuild;
	float Healing = 0.0f;
	if (Snapshot.IsValid() && Snapshot->CardCatalog.IsValid() &&
	    TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        const FReEchoCardEventResult Event = ReEchoCardRuntime::OnNegativeStatusApplied(
		            *Snapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats, StatusId, bAppliedByEcho);
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

void UReEchoRunSubsystem::NotifyCardEchoDefeated()
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	FReEchoBuildSnapshot PendingBuild;
	EReEchoHealthAdjustment HealthAdjustment = EReEchoHealthAdjustment::None;
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
		        HealthAdjustment = Event.HealthAdjustment;
		        return true;
	        },
	        PendingBuild))
	{
		CurrentBuild = PendingBuild;
		CommitCardHealthAdjustment(PreviousStats, HealthAdjustment);
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
	if (CurrentBuild.CardState.Runtime.FreeShopEncounterIndex == EncounterIndex)
	{
		return 0;
	}
	return FMath::Max(0, FMath::CeilToInt(BasePrice * (1.0f - GetCardRules().ShopDiscount)));
}

void UReEchoRunSubsystem::CommitCardHealthAdjustment(const FReEchoStatBlock& PreviousStats,
                                                     const EReEchoHealthAdjustment RequestedAdjustment)
{
	EReEchoHealthAdjustment EffectiveAdjustment = RequestedAdjustment;
	if (EffectiveAdjustment == EReEchoHealthAdjustment::None &&
	    DidCommittedHealthChange(PreviousStats, CurrentBuild.Stats))
	{
		EffectiveAdjustment = EReEchoHealthAdjustment::SetToStatPoint;
	}
	if (EffectiveAdjustment != EReEchoHealthAdjustment::None)
	{
		OnCardHealthCommitted.Broadcast(CurrentBuild.Stats, EffectiveAdjustment);
	}
}

EReEchoHealthAdjustment UReEchoRunSubsystem::ReevaluateCoreCollectionCard()
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return EReEchoHealthAdjustment::None;
	}
	TSet<FName> DistinctCoreIds;
	for (const FName PartId : OwnedPartIds)
	{
		const FReEchoCsvPartRow* Part = Snapshot->Parts.Find(PartId);
		if (Part && Part->bEnabled && Part->SlotTypeId == TEXT("Core"))
		{
			DistinctCoreIds.Add(PartId);
		}
	}
	FReEchoBuildSnapshot PendingBuild;
	EReEchoHealthAdjustment HealthAdjustment = EReEchoHealthAdjustment::None;
	if (TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& Build)
	        {
		        const FReEchoCardEventResult Event = ReEchoCardRuntime::OnCoreInventoryChanged(
		            *Snapshot->CardCatalog, Build.CardState, Build.Stats, DistinctCoreIds.Num());
		        Build.CardState = Event.CardState;
		        Build.Stats = Event.Stats;
		        HealthAdjustment = Event.HealthAdjustment;
		        return true;
	        },
	        PendingBuild))
	{
		CurrentBuild = MoveTemp(PendingBuild);
	}
	return HealthAdjustment;
}

bool UReEchoRunSubsystem::TryRefreshWeaponRuneShop(FString& OutError)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvShopRefreshRuleRow* RefreshRule =
	    Snapshot.IsValid() ? Snapshot->ShopRefreshRules.Find(TEXT("Default")) : nullptr;
	if (!RefreshRule)
	{
		OutError = TEXT("The Default shop refresh rule is unavailable");
		return false;
	}
	GetWeaponPartShopView(); // Lazily resets the per-encounter budget before validation.
	const FReEchoCardRuleSnapshot Rules = GetCardRules();
	if (Rules.bDisableShopRefresh)
	{
		OutError = TEXT("The current card rules disable shop refreshes");
		return false;
	}
	// G_4_1 grants free refreshes permanently once the shard balance has reached its threshold, so it takes
	// the free branch as well but never spends the finite free-refresh budget granted by other cards.
	const bool bEasterFreeRefresh = CurrentBuild.CardState.Runtime.bEasterUnlimitedRefreshUnlocked;
	const bool bUsesFreeRefresh = bEasterFreeRefresh || CurrentBuild.CardState.Runtime.FreeShopRefreshes > 0;
	if (bUsesFreeRefresh)
	{
		if (!bEasterFreeRefresh)
		{
			--CurrentBuild.CardState.Runtime.FreeShopRefreshes;
		}
	}
	else
	{
		if (!CurrentBuild.CardState.Runtime.bUnlimitedWeaponRuneRefresh &&
		    WeaponRuneRefreshesUsed >= RefreshRule->WeaponRuneRefreshLimit)
		{
			OutError = TEXT("The weapon/rune refresh budget is exhausted for this shop encounter");
			return false;
		}
		if (!CanPayShopCost(RefreshRule->WeaponRuneRefreshCost))
		{
			OutError = FString::Printf(TEXT("Time shards %d are below the weapon/rune refresh cost %d"),
			                           TimeShards,
			                           RefreshRule->WeaponRuneRefreshCost);
			return false;
		}
		CommitShopCost(RefreshRule->WeaponRuneRefreshCost);
		++WeaponRuneRefreshesUsed;
	}
	++WeaponRuneRefreshSequence;
	OutError.Reset();
	return true;
}

bool UReEchoRunSubsystem::TryRefreshShopCardSlot(const int32 Tier, const int32 SlotIndex, FString& OutError)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvShopRefreshRuleRow* RefreshRule =
	    Snapshot.IsValid() ? Snapshot->ShopRefreshRules.Find(TEXT("Default")) : nullptr;
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid() || !RefreshRule)
	{
		OutError = TEXT("Shop card refresh data is unavailable");
		return false;
	}
	if (GetCardRules().bDisableShopRefresh)
	{
		OutError = TEXT("The current card rules disable shop refreshes");
		return false;
	}
	GetWeaponPartShopView(); // Ensures the current encounter's fixed tier packs exist.
	const int32 PackIndex = Tier - 1;
	if (!CurrentBuild.CardState.Runtime.ShopCardPackStates.IsValidIndex(PackIndex))
	{
		OutError = TEXT("The requested card tier is not present on the current shop page");
		return false;
	}
	FReEchoShopCardPackRuntimeState& Pack = CurrentBuild.CardState.Runtime.ShopCardPackStates[PackIndex];
	if (Pack.Tier != Tier || !Pack.bPaymentCommitted || Pack.bPurchased ||
	    !Pack.CandidateCardIds.IsValidIndex(SlotIndex) || !Pack.SlotRefreshUses.IsValidIndex(SlotIndex))
	{
		OutError = TEXT("The requested card slot is no longer available");
		return false;
	}
	const int32 CurrentUses = Pack.SlotRefreshUses[SlotIndex];
	if (CurrentUses >= RefreshRule->CardSlotRefreshLimit)
	{
		OutError = TEXT("This card slot has already used its refresh opportunity");
		return false;
	}

	if (!CanPayShopCost(RefreshRule->CardSlotRefreshCost))
	{
		OutError = FString::Printf(TEXT("Time shards %d are below the card-slot refresh cost %d"),
		                           TimeShards,
		                           RefreshRule->CardSlotRefreshCost);
		return false;
	}

	const int32 NextSequence = CurrentUses + 1;
	const FName SlotSeedKey(*FString::Printf(TEXT("SHOP_CARD_TIER_%d_SLOT_%d"), Tier, SlotIndex));
	const FName ReplacementCardId =
	    SelectCardForOfferSlot(*Snapshot->CardCatalog,
	                           CurrentBuild.CardState,
	                           Tier,
	                           EncounterIndex,
	                           Pack.OfferHistoryCardIds,
	                           BuildShopOfferSeed(RunSeed, SlotSeedKey, EncounterIndex, NextSequence),
	                           false);
	if (ReplacementCardId.IsNone())
	{
		OutError = TEXT("No legal unowned same-tier or Easter replacement remains");
		return false;
	}
	const FName PreviousCardId = Pack.CandidateCardIds[SlotIndex];
	CommitShopCost(RefreshRule->CardSlotRefreshCost);
	Pack.CandidateCardIds[SlotIndex] = ReplacementCardId;
	Pack.OfferHistoryCardIds.AddUnique(ReplacementCardId);
	Pack.SlotRefreshUses[SlotIndex] = NextSequence;
	OutError.Reset();
	UE_LOG(
	    LogReEcho,
	    Warning,
	    TEXT("[ShopCardRefresh] encounter=%d tier=%d slot=%d previous=%s replacement=%s uses=%d remaining=%d cost=%d"),
	    EncounterIndex,
	    Tier,
	    SlotIndex,
	    *PreviousCardId.ToString(),
	    *ReplacementCardId.ToString(),
	    NextSequence,
	    FMath::Max(0, RefreshRule->CardSlotRefreshLimit - NextSequence),
	    RefreshRule->CardSlotRefreshCost);
	return true;
}

bool UReEchoRunSubsystem::TryConsumeShopRefresh(const int32 PaidRefreshPrice)
{
	(void)PaidRefreshPrice;
	FString Error;
	return TryRefreshWeaponRuneShop(Error);
}

bool UReEchoRunSubsystem::CanPurchaseExtraShopCard() const
{
	return !GetCardRules().bDisableExtraCardPurchase;
}

FReEchoShopPurchaseOutcome UReEchoRunSubsystem::PurchaseShopCardPackDetailed(const int32 Tier)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	const FReEchoWeaponPartShopView ShopView = GetWeaponPartShopView();
	const FReEchoShopCardPackOffer* Offer = ShopView.CardPackOffers.FindByPredicate(
	    [Tier](const FReEchoShopCardPackOffer& Candidate)
	    {
		    return Candidate.Tier == Tier;
	    });
	const FName AuditItemId = Offer ? Offer->ItemId : FName(*FString::Printf(TEXT("SHOP_CARD_PACK_T%d"), Tier));
	const FString TransactionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	LogShopPurchaseAuditState(
	    TransactionId, AuditItemId, TEXT("BEFORE"), CaptureShopPurchaseAuditState(*this, Snapshot));
	const auto Finish = [&](const EReEchoShopPurchaseResult Result, const FString& Detail, const int32 Price = 0)
	{
		FReEchoShopPurchaseOutcome Outcome;
		Outcome.TransactionId = TransactionId;
		Outcome.ItemId = AuditItemId;
		Outcome.Result = Result;
		Outcome.Detail = Detail;
		Outcome.EffectivePrice = Price;
		WriteShopPurchaseAuditLine(FString::Printf(
		    TEXT("[ShopPurchaseAudit] tx=%s phase=RESULT item=%s success=%d code=%s effectivePrice=%d detail=%s"),
		    *TransactionId,
		    *AuditItemId.ToString(),
		    Outcome.IsSuccess(),
		    GetShopPurchaseResultName(Result),
		    Price,
		    *SanitizeShopPurchaseAuditText(Detail)));
		LogShopPurchaseAuditState(
		    TransactionId, AuditItemId, TEXT("AFTER"), CaptureShopPurchaseAuditState(*this, GetRunDataSnapshot()));
		return Outcome;
	};
	if (!Offer || !Offer->IsAvailable())
	{
		return Finish(EReEchoShopPurchaseResult::OfferNotFound,
		              TEXT("The requested card pack is not available for payment"));
	}
	const int32 EffectivePrice = GetDiscountedShopPrice(Offer->Price);
	if (!CanPurchaseExtraShopCard())
	{
		return Finish(EReEchoShopPurchaseResult::PurchaseDisabled,
		              TEXT("The current card rules disable extra shop-card purchases"),
		              EffectivePrice);
	}
	if (!CanPayShopCost(EffectivePrice))
	{
		return Finish(
		    EReEchoShopPurchaseResult::InsufficientCurrency,
		    FString::Printf(TEXT("Time shards %d are below the effective pack price %d"), TimeShards, EffectivePrice),
		    EffectivePrice);
	}
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return Finish(
		    EReEchoShopPurchaseResult::DataUnavailable, TEXT("The runtime card data is unavailable"), EffectivePrice);
	}
	const int32 RequestedSelectableCardCount = FMath::Max(1, ResolvePackSelectableCardCount(Tier));

	FReEchoBuildSnapshot PendingBuild;
	EReEchoHealthAdjustment PendingHealthAdjustment = EReEchoHealthAdjustment::None;
	FString MutationDetail = TEXT("The authoritative build rejected the card-pack payment");
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& Build)
	        {
		        const int32 PackIndex = Tier - 1;
		        if (!Build.CardState.Runtime.ShopCardPackStates.IsValidIndex(PackIndex))
		        {
			        return false;
		        }
		        FReEchoShopCardPackRuntimeState& Pack = Build.CardState.Runtime.ShopCardPackStates[PackIndex];
		        if (Pack.Tier != Tier || Pack.bPaymentCommitted || Pack.bPurchased || Pack.RemainingPurchases <= 0)
		        {
			        MutationDetail = TEXT("The card pack payment state changed before commit");
			        return false;
		        }
		        RollShopCardPackCandidates(*Snapshot, Build.CardState, Pack, Tier);
		        if (Pack.CandidateCardIds.IsEmpty())
		        {
			        MutationDetail = TEXT("No eligible card remains in the requested tier pack");
			        return false;
		        }
		        Pack.SelectableCardCount = FMath::Clamp(RequestedSelectableCardCount, 1, Pack.CandidateCardIds.Num());
		        Pack.PicksRemaining = Pack.SelectableCardCount;
		        Pack.bPaymentCommitted = true;
		        const FReEchoCardEventResult Event =
		            ReEchoCardRuntime::OnPurchase(*Snapshot->CardCatalog, Build.CardState, Build.Stats);
		        Build.CardState = Event.CardState;
		        Build.Stats = Event.Stats;
		        PendingHealthAdjustment = Event.HealthAdjustment;
		        return true;
	        },
	        PendingBuild))
	{
		return Finish(EReEchoShopPurchaseResult::MutationRejected, MutationDetail, EffectivePrice);
	}
	CurrentBuild = MoveTemp(PendingBuild);
	CommitShopCost(EffectivePrice);
	CommitCardHealthAdjustment(PreviousStats, PendingHealthAdjustment);
	ReEchoBuildTrace::LogSnapshot(
	    TEXT("ShopCardPackPaid"), EncounterIndex, Phase, CurrentBuild, FString::Printf(TEXT("tier=%d"), Tier));
	return Finish(EReEchoShopPurchaseResult::Succeeded, TEXT("Card-pack payment committed"), EffectivePrice);
}

FReEchoShopPurchaseOutcome UReEchoRunSubsystem::ClaimPaidShopCardChoices(const TArray<FName>& ItemIds)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	// Cadence abilities (the Sage) can let a paid pack hand out two cards. Both are granted inside one
	// authoritative mutation so the pack closes once, the cadence counter advances once, and the run can
	// never be left in a pending-choice state with no screen to interact with.
	if (ItemIds.Num() <= 0)
	{
		FReEchoShopPurchaseOutcome Outcome;
		Outcome.Result = EReEchoShopPurchaseResult::OfferNotFound;
		Outcome.Detail = TEXT("No card was supplied for the claim");
		return Outcome;
	}
	TSet<FName> UniqueItemIds;
	for (const FName& ItemId : ItemIds)
	{
		if (ItemId.IsNone() || UniqueItemIds.Contains(ItemId))
		{
			FReEchoShopPurchaseOutcome Outcome;
			Outcome.Result = EReEchoShopPurchaseResult::OfferNotFound;
			Outcome.Detail = TEXT("Card choices must be unique");
			return Outcome;
		}
		UniqueItemIds.Add(ItemId);
	}
	const FName PrimaryItemId = ItemIds[0];
	const FString TransactionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const auto Finish = [&](const EReEchoShopPurchaseResult Result, const FString& Detail)
	{
		FReEchoShopPurchaseOutcome Outcome;
		Outcome.TransactionId = TransactionId;
		Outcome.ItemId = PrimaryItemId;
		Outcome.Result = Result;
		Outcome.Detail = Detail;
		WriteShopPurchaseAuditLine(FString::Printf(
		    TEXT("[ShopPurchaseAudit] tx=%s phase=RESULT item=%s success=%d code=%s effectivePrice=0 detail=%s"),
		    *TransactionId,
		    *PrimaryItemId.ToString(),
		    Outcome.IsSuccess(),
		    GetShopPurchaseResultName(Result),
		    *SanitizeShopPurchaseAuditText(Detail)));
		return Outcome;
	};
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return Finish(EReEchoShopPurchaseResult::DataUnavailable, TEXT("The runtime card data is unavailable"));
	}
	const FReEchoWeaponPartShopView ShopView = GetWeaponPartShopView();
	int32 PackIndex = INDEX_NONE;
	TArray<FReEchoShopCardChoiceOffer> Choices;
	for (int32 Index = 0; Index < ShopView.CardPackOffers.Num() && PackIndex == INDEX_NONE; ++Index)
	{
		const FReEchoShopCardPackOffer& Pack = ShopView.CardPackOffers[Index];
		if (Pack.Status != EReEchoShopCardPackStatus::PaidPendingChoice)
		{
			continue;
		}
		TArray<FReEchoShopCardChoiceOffer> Found;
		for (const FName& ItemId : ItemIds)
		{
			const FReEchoShopCardChoiceOffer* Match = Pack.Choices.FindByPredicate(
			    [ItemId](const FReEchoShopCardChoiceOffer& Candidate)
			    {
				    return Candidate.ItemId == ItemId;
			    });
			if (!Match)
			{
				Found.Reset();
				break;
			}
			Found.Add(*Match);
		}
		if (Found.Num() == ItemIds.Num())
		{
			Choices = MoveTemp(Found);
			PackIndex = Index;
		}
	}
	if (PackIndex == INDEX_NONE)
	{
		return Finish(EReEchoShopPurchaseResult::OfferNotFound,
		              TEXT("The requested cards are not claimable from a paid pack"));
	}
	const FReEchoShopCardPackOffer& SelectedPack = ShopView.CardPackOffers[PackIndex];
	if (ItemIds.Num() != SelectedPack.SelectableCardCount)
	{
		return Finish(EReEchoShopPurchaseResult::OfferNotFound,
		              TEXT("The paid card pack requires an exact number of choices"));
	}
	for (const FReEchoShopCardChoiceOffer& Choice : Choices)
	{
		const FReEchoCardDefinition* ChoiceDefinition = Snapshot->CardCatalog->Find(Choice.CardId);
		// Egao Party: repeatable cards may be claimed again even while owned.
		if (ReEchoCardRuntime::HasCard(CurrentBuild.CardState, Choice.CardId) &&
		    (!ChoiceDefinition || ChoiceDefinition->StackPolicy == TEXT("Unique") || ChoiceDefinition->Tier != 1) &&
		    !(ChoiceDefinition && ReEchoCardRuntime::IsRepeatableCard(*ChoiceDefinition)))
		{
			return Finish(EReEchoShopPurchaseResult::AlreadyOwned,
			              TEXT("Owned unique, tier-2, tier-3, and Easter cards cannot be claimed again"));
		}
	}

	int32 PendingTimeShards = TimeShards;
	bool bPendingClearWeaponRunes = false;
	EReEchoHealthAdjustment PendingHealthAdjustment = EReEchoHealthAdjustment::None;
	FReEchoBuildSnapshot PendingBuild;
	EReEchoShopPurchaseResult Failure = EReEchoShopPurchaseResult::MutationRejected;
	FString FailureDetail = TEXT("The authoritative build rejected the card claim");
	const int32 ClaimPackIndex = PackIndex;
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& Build)
	        {
		        if (!Build.CardState.Runtime.ShopCardPackStates.IsValidIndex(ClaimPackIndex))
		        {
			        return false;
		        }
		        FReEchoShopCardPackRuntimeState& Pack = Build.CardState.Runtime.ShopCardPackStates[ClaimPackIndex];
		        if (!Pack.bPaymentCommitted || Pack.bPurchased)
		        {
			        FailureDetail = TEXT("The paid card-pack state changed before claim commit");
			        return false;
		        }
		        for (const FReEchoShopCardChoiceOffer& Choice : Choices)
		        {
			        if (!Pack.CandidateCardIds.Contains(Choice.CardId))
			        {
				        FailureDetail = TEXT("The paid card-pack state changed before claim commit");
				        return false;
			        }
			        FReEchoCardGrantInput Input;
			        Input.Stats = Build.Stats;
			        Input.CardState = Build.CardState;
			        Input.TimeShards = PendingTimeShards;
			        Input.EncounterIndex = EncounterIndex;
			        Input.RandomSeed =
			            BuildShopOfferSeed(RunSeed, Choice.CardId, EncounterIndex, Choice.SlotRefreshSequence);
			        const FReEchoCardGrantResult Grant =
			            ReEchoCardRuntime::TryGrantCard(*Snapshot->CardCatalog, Choice.CardId, Input);
			        if (!Grant.bSucceeded)
			        {
				        Failure = EReEchoShopPurchaseResult::GrantRejected;
				        FailureDetail = Grant.Error.IsEmpty() ? TEXT("The card grant was rejected") : Grant.Error;
				        return false;
			        }
			        Build.Stats = Grant.Stats;
			        Build.CardState = Grant.CardState;
			        PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, Grant.HealthAdjustment);
			        PendingTimeShards = Grant.TimeShards;
			        bPendingClearWeaponRunes |= Grant.bClearWeaponRunes;
			        if (Grant.bClearWeaponRunes)
			        {
				        Build.EquippedParts.Reset();
			        }
		        }
		        if (!Build.CardState.Runtime.ShopCardPackStates.IsValidIndex(ClaimPackIndex))
		        {
			        FailureDetail = TEXT("The card grant did not preserve the paid shop pack");
			        return false;
		        }
		        FReEchoShopCardPackRuntimeState& ClaimedPack =
		            Build.CardState.Runtime.ShopCardPackStates[ClaimPackIndex];
		        // One pack, one cadence step, regardless of how many cards it yields.
		        ClaimedPack.PicksRemaining = 0;
		        ClaimedPack.bPurchased = true;
		        if (ClaimedPack.RemainingPurchases > 1)
		        {
			        ClaimedPack.RemainingPurchases--;
			        ClaimedPack.bPurchased = false;
			        ClaimedPack.bPaymentCommitted = false;
			        ClaimedPack.ClaimCount++;
			        ClaimedPack.CandidateCardIds.Reset();
			        ClaimedPack.OfferHistoryCardIds.Reset();
			        ClaimedPack.SlotRefreshUses.Reset();
		        }
		        else
		        {
			        ClaimedPack.RemainingPurchases = 0;
		        }
		        RecordNormalTraitGroupSelection(*Snapshot, Build, ClaimedPack.Tier);
		        return true;
	        },
	        PendingBuild))
	{
		return Finish(Failure, FailureDetail);
	}
	const int32 PreviousTimeShards = TimeShards;
	CurrentBuild = MoveTemp(PendingBuild);
	ApplyProjectedCardCurrency(PreviousTimeShards, PendingTimeShards);
	if (bPendingClearWeaponRunes)
	{
		OwnedPartIds.Reset();
		RuneAcquisitionCounts.Reset();
	}
	PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, ReevaluateCoreCollectionCard());
	for (const FName& ItemId : ItemIds)
	{
		InventoryItems.AddUnique(ItemId);
	}
	ReEchoBuildTrace::LogSnapshot(TEXT("PaidCardsGranted"),
	                              EncounterIndex,
	                              Phase,
	                              CurrentBuild,
	                              FString::Printf(TEXT("count=%d"), ItemIds.Num()));
	CommitCardHealthAdjustment(PreviousStats, PendingHealthAdjustment);
	return Finish(EReEchoShopPurchaseResult::Succeeded,
	              FString::Printf(TEXT("Claimed %d choices from the paid card pack"), ItemIds.Num()));
}

FReEchoShopPurchaseOutcome UReEchoRunSubsystem::ClaimPaidShopCardChoice(const FName ItemId)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
	const FString TransactionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	LogShopPurchaseAuditState(TransactionId, ItemId, TEXT("BEFORE"), CaptureShopPurchaseAuditState(*this, Snapshot));
	const auto Finish = [&](const EReEchoShopPurchaseResult Result, const FString& Detail)
	{
		FReEchoShopPurchaseOutcome Outcome;
		Outcome.TransactionId = TransactionId;
		Outcome.ItemId = ItemId;
		Outcome.Result = Result;
		Outcome.Detail = Detail;
		WriteShopPurchaseAuditLine(FString::Printf(
		    TEXT("[ShopPurchaseAudit] tx=%s phase=RESULT item=%s success=%d code=%s effectivePrice=0 detail=%s"),
		    *TransactionId,
		    *ItemId.ToString(),
		    Outcome.IsSuccess(),
		    GetShopPurchaseResultName(Result),
		    *SanitizeShopPurchaseAuditText(Detail)));
		LogShopPurchaseAuditState(
		    TransactionId, ItemId, TEXT("AFTER"), CaptureShopPurchaseAuditState(*this, GetRunDataSnapshot()));
		return Outcome;
	};
	if (!Snapshot.IsValid() || !Snapshot->CardCatalog.IsValid())
	{
		return Finish(EReEchoShopPurchaseResult::DataUnavailable, TEXT("The runtime card data is unavailable"));
	}
	const FReEchoWeaponPartShopView ShopView = GetWeaponPartShopView();
	FReEchoShopCardChoiceOffer Choice;
	int32 PackIndex = INDEX_NONE;
	for (int32 Index = 0; Index < ShopView.CardPackOffers.Num() && PackIndex == INDEX_NONE; ++Index)
	{
		const FReEchoShopCardPackOffer& Pack = ShopView.CardPackOffers[Index];
		if (Pack.Status != EReEchoShopCardPackStatus::PaidPendingChoice)
		{
			continue;
		}
		if (const FReEchoShopCardChoiceOffer* Found = Pack.Choices.FindByPredicate(
		        [ItemId](const FReEchoShopCardChoiceOffer& Candidate)
		        {
			        return Candidate.ItemId == ItemId;
		        }))
		{
			Choice = *Found;
			PackIndex = Index;
		}
	}
	if (PackIndex == INDEX_NONE)
	{
		return Finish(EReEchoShopPurchaseResult::OfferNotFound,
		              TEXT("The requested card is not claimable from a paid pack"));
	}
	if (ShopView.CardPackOffers[PackIndex].SelectableCardCount != 1)
	{
		return Finish(EReEchoShopPurchaseResult::OfferNotFound, TEXT("This paid card pack requires multiple choices"));
	}
	const FReEchoCardDefinition* ChoiceDefinition = Snapshot->CardCatalog->Find(Choice.CardId);
	// Egao Party: repeatable cards may be claimed again even while owned.
	if (ReEchoCardRuntime::HasCard(CurrentBuild.CardState, Choice.CardId) &&
	    (!ChoiceDefinition || ChoiceDefinition->StackPolicy == TEXT("Unique") || ChoiceDefinition->Tier != 1) &&
	    !(ChoiceDefinition && ReEchoCardRuntime::IsRepeatableCard(*ChoiceDefinition)))
	{
		return Finish(EReEchoShopPurchaseResult::AlreadyOwned,
		              TEXT("Owned unique, tier-2, tier-3, and Easter cards cannot be claimed again"));
	}

	int32 PendingTimeShards = TimeShards;
	bool bPendingClearWeaponRunes = false;
	bool bPendingBonusTraitChoice = false;
	EReEchoHealthAdjustment PendingHealthAdjustment = EReEchoHealthAdjustment::None;
	FReEchoBuildSnapshot PendingBuild;
	EReEchoShopPurchaseResult Failure = EReEchoShopPurchaseResult::MutationRejected;
	FString FailureDetail = TEXT("The authoritative build rejected the card claim");
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& Build)
	        {
		        if (!Build.CardState.Runtime.ShopCardPackStates.IsValidIndex(PackIndex))
		        {
			        return false;
		        }
		        FReEchoShopCardPackRuntimeState& Pack = Build.CardState.Runtime.ShopCardPackStates[PackIndex];
		        if (!Pack.bPaymentCommitted || Pack.bPurchased || !Pack.CandidateCardIds.Contains(Choice.CardId))
		        {
			        FailureDetail = TEXT("The paid card-pack state changed before claim commit");
			        return false;
		        }
		        FReEchoCardGrantInput Input;
		        Input.Stats = Build.Stats;
		        Input.CardState = Build.CardState;
		        Input.TimeShards = PendingTimeShards;
		        Input.EncounterIndex = EncounterIndex;
		        Input.RandomSeed =
		            BuildShopOfferSeed(RunSeed, Choice.CardId, EncounterIndex, Choice.SlotRefreshSequence);
		        const FReEchoCardGrantResult Grant =
		            ReEchoCardRuntime::TryGrantCard(*Snapshot->CardCatalog, Choice.CardId, Input);
		        if (!Grant.bSucceeded)
		        {
			        Failure = EReEchoShopPurchaseResult::GrantRejected;
			        FailureDetail = Grant.Error.IsEmpty() ? TEXT("The card grant was rejected") : Grant.Error;
			        return false;
		        }
		        Build.Stats = Grant.Stats;
		        Build.CardState = Grant.CardState;
		        PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, Grant.HealthAdjustment);
		        bPendingClearWeaponRunes |= Grant.bClearWeaponRunes;
		        if (Grant.bClearWeaponRunes)
		        {
			        Build.EquippedParts.Reset();
		        }
		        if (!Build.CardState.Runtime.ShopCardPackStates.IsValidIndex(PackIndex))
		        {
			        FailureDetail = TEXT("The card grant did not preserve the paid shop pack");
			        return false;
		        }
		        FReEchoShopCardPackRuntimeState& ClaimedPack = Build.CardState.Runtime.ShopCardPackStates[PackIndex];
		        // NOTE: see ApplyTraitCard. The pack closes on the first pick until the multi-select choice
		        // screen exists; keeping it open without reopening the UI would strand the run.
		        ClaimedPack.PicksRemaining = 0;
		        ClaimedPack.bPurchased = true;
		        {
			        // Multi-purchase support: if remaining > 1, reset for next purchase cycle
			        if (ClaimedPack.RemainingPurchases > 1)
			        {
				        ClaimedPack.RemainingPurchases--;
				        // Reset purchase state so this tier can be paid again.
				        ClaimedPack.bPurchased = false;
				        ClaimedPack.bPaymentCommitted = false;
				        // The next purchase generates its candidates against the then-current owned state.
				        ClaimedPack.ClaimCount++;
				        ClaimedPack.CandidateCardIds.Reset();
				        ClaimedPack.OfferHistoryCardIds.Reset();
				        ClaimedPack.SlotRefreshUses.Reset();
			        }
			        else
			        {
				        ClaimedPack.RemainingPurchases = 0; // fully consumed
			        }
		        } // end of the single-pick pack close
		        bPendingBonusTraitChoice = RecordNormalTraitGroupSelection(*Snapshot, Build, ClaimedPack.Tier) > 0;
		        PendingTimeShards = Grant.TimeShards;
		        return true;
	        },
	        PendingBuild))
	{
		return Finish(Failure, FailureDetail);
	}
	const int32 PreviousTimeShards = TimeShards;
	CurrentBuild = MoveTemp(PendingBuild);
	ApplyProjectedCardCurrency(PreviousTimeShards, PendingTimeShards);
	if (bPendingBonusTraitChoice)
	{
		SetPhase(EReEchoRunPhase::CardChoice);
	}
	if (bPendingClearWeaponRunes)
	{
		OwnedPartIds.Reset();
		RuneAcquisitionCounts.Reset();
	}
	PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, ReevaluateCoreCollectionCard());
	InventoryItems.AddUnique(ItemId);
	ReEchoBuildTrace::LogSnapshot(
	    TEXT("PaidCardGranted"),
	    EncounterIndex,
	    Phase,
	    CurrentBuild,
	    FString::Printf(TEXT("card=%s item=%s"), *Choice.CardId.ToString(), *ItemId.ToString()));
	CommitCardHealthAdjustment(PreviousStats, PendingHealthAdjustment);
	return Finish(EReEchoShopPurchaseResult::Succeeded, TEXT("Paid card choice claimed"));
}

FReEchoShopPurchaseOutcome UReEchoRunSubsystem::PurchaseShopItemDetailed(const FName ItemId)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
	const FReEchoStatBlock PreviousStats = CurrentBuild.Stats;
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

	// Card packs use their dedicated payment and claim commands; this lookup is weapon/rune and legacy only.
	if (Snapshot && Snapshot->Parts.Contains(ItemId))
	{
		TArray<FName> AllOwned = OwnedPartIds;
		for (const FReEchoEquippedPartSnapshot& Part : CurrentBuild.EquippedParts)
		{
			AllOwned.AddUnique(Part.PartId);
		}
		if (PurchasedWeaponPartOfferIds.Contains(ItemId) ||
		    ReEchoRuneInventory::OwnsTerminalTier(*Snapshot, ItemId, AllOwned))
		{
			return FinishPurchase(EReEchoShopPurchaseResult::AlreadyOwned,
			                      TEXT("Offer already consumed on this page or terminal rune tier already owned"));
		}
	}
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
	// Legacy rune-item catalog (SHOP_RUSTED_SCISSORS etc.) kept for backward compatibility.
	EReEchoShopOfferType LegacyType = EReEchoShopOfferType::RunItem;
	int32 LegacyPrice = 0;
	bool bIsLegacy = false;
	if (!bFoundSlot)
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
	if (!bFoundSlot && !bIsLegacy)
	{
		return FinishPurchase(EReEchoShopPurchaseResult::OfferNotFound,
		                      TEXT("The requested item is not present on the current shop page"));
	}

	// Price + free handling.
	int32 RawPrice = 0;
	if (bFoundSlot)
	{
		RawPrice = SlotOffer.Price;
	}
	else if (bIsLegacy)
	{
		RawPrice = LegacyPrice;
	}
	const int32 EffectivePrice = GetDiscountedShopPrice(RawPrice);

	if (InventoryItems.Contains(ItemId))
	{
		return FinishPurchase(EReEchoShopPurchaseResult::AlreadyOwned,
		                      TEXT("The requested shop item was already purchased"),
		                      EffectivePrice);
	}
	// A tier-I rune may be bought again to feed I->II->III synthesis, so owning one does not reject the
	// purchase. Every other part keeps the legacy "buy once" rule.
	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Part && OwnedPartIds.Contains(SlotOffer.PartId) &&
	    !SlotOffer.bRepeatPurchasable)
	{
		return FinishPurchase(
		    EReEchoShopPurchaseResult::AlreadyOwned, TEXT("The requested rune is already owned"), EffectivePrice);
	}
	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Weapon && OwnedWeaponIds.Contains(SlotOffer.WeaponId))
	{
		return FinishPurchase(
		    EReEchoShopPurchaseResult::AlreadyOwned, TEXT("The requested weapon is already owned"), EffectivePrice);
	}
	if (!CanPayShopCost(EffectivePrice))
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
	FReEchoBuildSnapshot PendingBuild;
	EReEchoHealthAdjustment PendingHealthAdjustment = EReEchoHealthAdjustment::None;
	FString MutationFailureDetail = TEXT("The authoritative build rejected the purchase mutation");
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        if (bIsLegacy && LegacyType == EReEchoShopOfferType::RunItem)
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
		        }
		        if (Snapshot->CardCatalog.IsValid())
		        {
			        const FReEchoCardEventResult Event =
			            ReEchoCardRuntime::OnPurchase(*Snapshot->CardCatalog, BaseBuild.CardState, BaseBuild.Stats);
			        BaseBuild.CardState = Event.CardState;
			        BaseBuild.Stats = Event.Stats;
			        PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, Event.HealthAdjustment);
		        }
		        return true;
	        },
	        PendingBuild))
	{
		return FinishPurchase(EReEchoShopPurchaseResult::MutationRejected, MutationFailureDetail, EffectivePrice);
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

	FString CompletionDetail = TEXT("Purchase committed");
	TArray<FName> PendingOwned = OwnedPartIds;
	TMap<FName, int32> PendingCounts =
	    NormalizeRuneCounts(OwnedPartIds, CurrentBuild.EquippedParts, RuneAcquisitionCounts);
	for (const FReEchoEquippedPartSnapshot& Part : CurrentBuild.EquippedParts)
	{
		PendingOwned.AddUnique(Part.PartId);
	}
	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Part)
	{
		const int32 Held = PendingCounts.FindRef(SlotOffer.PartId);
		if (Held == MAX_int32)
		{
			return FinishPurchase(
			    EReEchoShopPurchaseResult::MutationRejected, TEXT("Rune copy count overflow"), EffectivePrice);
		}
		PendingOwned.AddUnique(SlotOffer.PartId);
		PendingCounts.Add(SlotOffer.PartId, Held + 1);
	}
	if (bFoundSlot)
	{
		FString SynthesisError;
		const FName AcquiredId = SlotOffer.Kind == EReEchoShopOfferKind::Part ? SlotOffer.PartId : NAME_None;
		if (!TryResolveRuneInventory(*Snapshot,
		                             PendingBuild,
		                             PendingOwned,
		                             PendingCounts,
		                             AcquiredId,
		                             PendingBuild,
		                             PendingOwned,
		                             PendingCounts,
		                             SynthesisError))
		{
			return FinishPurchase(EReEchoShopPurchaseResult::MutationRejected, SynthesisError, EffectivePrice);
		}
	}
	// No currency, ownership or equipment changes escape until the complete candidate has validated.
	CurrentBuild = MoveTemp(PendingBuild);
	OwnedPartIds = MoveTemp(PendingOwned);
	RuneAcquisitionCounts = MoveTemp(PendingCounts);

	if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Weapon)
	{
		// PendingBuild already switched through WeaponRuntime so revisions, equipment base stats and compatible-rune
		// retention share the same transaction used by backpack weapon selection.
		OwnedWeaponIds.Add(SlotOffer.WeaponId);
	}
	else if (bFoundSlot && SlotOffer.Kind == EReEchoShopOfferKind::Part)
	{
		// Consume this slot for the current page; the rune can be offered again once the page regenerates.
		PurchasedWeaponPartOfferIds.Add(SlotOffer.PartId);
		CompletionDetail = TEXT("Purchase committed; backpack/equipped synthesis settled");
	}
	else if (bIsLegacy)
	{
		InventoryItems.Add(ItemId);
	}
	// Subscribers to the balance event must see the final inventory AND consumed-page record.
	CommitShopCost(EffectivePrice);

	PendingHealthAdjustment = MergeHealthAdjustments(PendingHealthAdjustment, ReevaluateCoreCollectionCard());
	ReEchoBuildTrace::LogSnapshot(TEXT("ShopPurchaseCommitted"),
	                              EncounterIndex,
	                              Phase,
	                              CurrentBuild,
	                              FString::Printf(TEXT("item=%s"), *ItemId.ToString()));
	CommitCardHealthAdjustment(PreviousStats, PendingHealthAdjustment);
	return FinishPurchase(EReEchoShopPurchaseResult::Succeeded, CompletionDetail, EffectivePrice);
}

bool UReEchoRunSubsystem::IsShopOfferRepeatPurchasable(const FName ItemId) const
{
	if (ItemId.IsNone())
	{
		return false;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> LoadedSnapshot = GetRunDataSnapshot();
	if (!LoadedSnapshot.IsValid())
	{
		return false;
	}
	// Only the tier-I rune of a family is ever sold, and it must stay buyable so the player can gather the
	// copies that synthesis consumes. Tiers II/III are never offered in the shop.
	return GetRuneTierFromPartId(ItemId) == 1 && LoadedSnapshot->RuneUpgrades.Contains(ItemId);
}

int32 UReEchoRunSubsystem::GetPaidShopCardPackSelectableCount() const
{
	for (const FReEchoShopCardPackRuntimeState& Pack : CurrentBuild.CardState.Runtime.ShopCardPackStates)
	{
		if (Pack.bPaymentCommitted && !Pack.bPurchased)
		{
			return FMath::Max(1, Pack.SelectableCardCount);
		}
	}
	return 1;
}

int32 UReEchoRunSubsystem::ResolvePackSelectableCardCount(const int32 CardPackTier) const
{
	// Evaluated before the choice screen opens: this pack is the next one to be claimed, so it lands on the
	// cadence when (already claimed + 1) is a multiple of the ability's interval and passes its tier gate.
	const TSharedPtr<const FReEchoCsvDataSnapshot> LoadedSnapshot = GetRunDataSnapshot();
	if (!LoadedSnapshot.IsValid())
	{
		return 1;
	}
	const int32 ClaimedSoFar =
	    FMath::Max(0, FCString::Atoi(*CurrentBuild.RuleFlags.FindRef(NormalTraitSelectionsFlag)));
	const int32 NextSelectionIndex = ClaimedSoFar + 1;
	return 1 + ReEchoCharacterAbilityRuntime::ResolveExtraTraitChoicesForTier(
	               *LoadedSnapshot, CurrentBuild.CharacterId, NextSelectionIndex, CardPackTier);
}

void UReEchoRunSubsystem::RollShopCardPackCandidates(const FReEchoCsvDataSnapshot& SnapshotRef,
                                                     FReEchoCardBuildState& CardState,
                                                     FReEchoShopCardPackRuntimeState& Pack,
                                                     const int32 Tier)
{
	// Candidate generation is part of payment: it observes every card granted earlier in this shop. ClaimCount
	// keeps repeated purchases independent while the resulting page remains stable until the claim completes.
	Pack.CandidateCardIds.Reset();
	Pack.OfferHistoryCardIds.Reset();
	Pack.SlotRefreshUses.Reset();
	if (!SnapshotRef.CardCatalog.IsValid())
	{
		return;
	}
	constexpr int32 RefreshSequence = 0;
	for (int32 CandidateIndex = 0; CandidateIndex < ReEchoShopOfferCountPerGroup; ++CandidateIndex)
	{
		const FName SlotSeedKey(
		    *FString::Printf(TEXT("SHOP_CARD_TIER_%d_SLOT_%d_ROLL_%d"), Tier, CandidateIndex, Pack.ClaimCount));
		const FName SelectedCardId =
		    SelectCardForOfferSlot(*SnapshotRef.CardCatalog,
		                           CardState,
		                           Tier,
		                           EncounterIndex,
		                           Pack.OfferHistoryCardIds,
		                           BuildShopOfferSeed(RunSeed, SlotSeedKey, EncounterIndex, RefreshSequence));
		if (SelectedCardId.IsNone())
		{
			break;
		}
		Pack.CandidateCardIds.Add(SelectedCardId);
		Pack.OfferHistoryCardIds.Add(SelectedCardId);
		Pack.SlotRefreshUses.Add(0);
	}
}

bool UReEchoRunSubsystem::PurchaseShopItem(const FName ItemId)
{
	return PurchaseShopItemDetailed(ItemId).IsSuccess();
}

bool UReEchoRunSubsystem::GrantTimeShards(const int32 Amount)
{
	// Combat pickups keep their existing HUD path; settlement wraps this helper in its outer transaction.
	const FScopedTimeShardBalanceChange BalanceChange(*this, Phase != EReEchoRunPhase::Encounter);
	if (Amount <= 0)
	{
		return false;
	}
	int32 Remaining = Amount;
	if (Phase == EReEchoRunPhase::Encounter)
	{
		CurrentBuild.CardState.Runtime.CurrentEncounterGrossShardIncome = static_cast<int32>(FMath::Min<int64>(
		    static_cast<int64>(CurrentBuild.CardState.Runtime.CurrentEncounterGrossShardIncome) + Amount,
		    TNumericLimits<int32>::Max()));
		if (ReEchoCardRuntime::HasCard(CurrentBuild.CardState, TEXT("G_4_8")))
		{
			Remaining = FMath::FloorToInt(
			    Remaining * FMath::Max(0.0f, CurrentBuild.CardState.Runtime.EncounterShardIncomeMultiplier));
		}
	}
	if (Remaining <= 0)
	{
		return true;
	}
	if (CurrentBuild.CardState.Runtime.TimeShardDebt > 0)
	{
		const int32 Repaid = FMath::Min(Remaining, CurrentBuild.CardState.Runtime.TimeShardDebt);
		CurrentBuild.CardState.Runtime.TimeShardDebt -= Repaid;
		Remaining -= Repaid;
		RefreshCurseBankOutcome();
	}
	if (Remaining <= 0)
	{
		return true;
	}
	if (TimeShards == TNumericLimits<int32>::Max())
	{
		return false;
	}

	const int64 GrantedTotal = static_cast<int64>(TimeShards) + static_cast<int64>(Remaining);
	TimeShards = static_cast<int32>(FMath::Min<int64>(GrantedTotal, TNumericLimits<int32>::Max()));
	return true;
}

bool UReEchoRunSubsystem::CanPayShopCost(const int32 Cost) const
{
	return Cost <= 0 || TimeShards >= Cost || GetCardRules().bUnlimitedShopCredit;
}

int32 UReEchoRunSubsystem::GetDisplayedTimeShardBalance() const
{
	return TimeShards - FMath::Max(0, CurrentBuild.CardState.Runtime.TimeShardDebt);
}

void UReEchoRunSubsystem::CommitShopCost(const int32 Cost)
{
	const int32 ClampedCost = FMath::Max(0, Cost);
	const int32 Paid = FMath::Min(TimeShards, ClampedCost);
	TimeShards -= Paid;
	const int32 Borrowed = ClampedCost - Paid;
	if (Borrowed > 0)
	{
		CurrentBuild.CardState.Runtime.TimeShardDebt = static_cast<int32>(FMath::Min<int64>(
		    static_cast<int64>(CurrentBuild.CardState.Runtime.TimeShardDebt) + Borrowed, TNumericLimits<int32>::Max()));
		RefreshCurseBankOutcome();
	}
}

void UReEchoRunSubsystem::ApplyProjectedCardCurrency(const int32 PreviousBalance,
                                                     const int32 ProjectedBalance,
                                                     const bool bApplyEncounterIncomeRules)
{
	if (ProjectedBalance > PreviousBalance)
	{
		TimeShards = PreviousBalance;
		if (bApplyEncounterIncomeRules)
		{
			GrantTimeShards(ProjectedBalance - PreviousBalance);
			return;
		}
		int32 Remaining = ProjectedBalance - PreviousBalance;
		if (CurrentBuild.CardState.Runtime.TimeShardDebt > 0)
		{
			const int32 Repaid = FMath::Min(Remaining, CurrentBuild.CardState.Runtime.TimeShardDebt);
			CurrentBuild.CardState.Runtime.TimeShardDebt -= Repaid;
			Remaining -= Repaid;
			RefreshCurseBankOutcome();
		}
		TimeShards = static_cast<int32>(
		    FMath::Min<int64>(static_cast<int64>(TimeShards) + Remaining, TNumericLimits<int32>::Max()));
	}
	else
	{
		TimeShards = FMath::Max(0, ProjectedBalance);
	}
}

void UReEchoRunSubsystem::RefreshCurseBankOutcome()
{
	if (!CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_2_19")))
	{
		return;
	}
	FReEchoCardOutcomeState* Outcome = CurrentBuild.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Candidate)
	    {
		    return Candidate.CardId == TEXT("G_2_19");
	    });
	if (!Outcome)
	{
		Outcome = &CurrentBuild.CardState.Runtime.ResolvedOutcomes.AddDefaulted_GetRef();
		Outcome->CardId = TEXT("G_2_19");
	}
	Outcome->Kind = EReEchoCardOutcomeKind::Debt;
	Outcome->PrimaryTarget = TEXT("TimeShards");
	Outcome->PrimaryValue = CurrentBuild.CardState.Runtime.TimeShardDebt;
	Outcome->ResolutionCount = EncounterIndex;
}

void UReEchoRunSubsystem::RefreshWeaponMasterOutcome()
{
	if (!CurrentBuild.CardState.OwnedCardIds.Contains(TEXT("G_3_28")))
	{
		return;
	}
	FReEchoCardOutcomeState* Outcome = CurrentBuild.CardState.Runtime.ResolvedOutcomes.FindByPredicate(
	    [](const FReEchoCardOutcomeState& Candidate)
	    {
		    return Candidate.CardId == TEXT("G_3_28");
	    });
	if (!Outcome)
	{
		Outcome = &CurrentBuild.CardState.Runtime.ResolvedOutcomes.AddDefaulted_GetRef();
		Outcome->CardId = TEXT("G_3_28");
	}
	const int32 WeaponCount = CurrentBuild.CardState.Runtime.MasteredWeaponIds.Num();
	Outcome->Kind = EReEchoCardOutcomeKind::WeaponMaster;
	Outcome->PrimaryTarget = TEXT("PhysicalAndElementalAttack");
	Outcome->PrimaryValue = WeaponCount * 5.0f;
	Outcome->SecondaryTarget = TEXT("HpMaxAndPoint");
	Outcome->SecondaryValue = WeaponCount * 10.0f;
	Outcome->ResolutionCount = WeaponCount;
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
	ClearTimeAnchorRecording();
}

void UReEchoRunSubsystem::ClearTimeAnchorRecording()
{
	bHasTimeAnchorRecording = false;
	TimeAnchorRecording = {};
	CurrentBuild.CardState.Runtime.bHasAnchorRecording = false;
	CurrentBuild.CardState.Runtime.AnchorRecordingId.Invalidate();
}

EReEchoEchoStorageResult UReEchoRunSubsystem::StagePendingRecording(const FReEchoRecording& Recording)
{
	if (!Recording.Id.IsValid())
	{
		return EReEchoEchoStorageResult::InvalidRecordingId;
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

EReEchoEchoStorageResult UReEchoRunSubsystem::StorePendingRecordingAsTimeAnchor()
{
	if (!bHasPendingRecording)
	{
		return EReEchoEchoStorageResult::NoPendingRecording;
	}
	if (!PendingRecording.Id.IsValid())
	{
		return EReEchoEchoStorageResult::InvalidRecordingId;
	}
	bHasTimeAnchorRecording = true;
	TimeAnchorRecording = PendingRecording;
	CurrentBuild.CardState.Runtime.bHasAnchorRecording = true;
	CurrentBuild.CardState.Runtime.AnchorRecordingId = PendingRecording.Id;
	bHasPendingRecording = false;
	PendingRecording = {};
	return EReEchoEchoStorageResult::Success;
}

FReEchoEchoStorageSummary UReEchoRunSubsystem::GetEchoStorageSummary() const
{
	FReEchoEchoStorageSummary Summary;
	Summary.bHasTimeAnchor = bHasTimeAnchorRecording && CurrentBuild.CardState.Runtime.bHasAnchorRecording &&
	                         TimeAnchorRecording.Id == CurrentBuild.CardState.Runtime.AnchorRecordingId;
	if (Summary.bHasTimeAnchor)
	{
		Summary.TimeAnchorRecording = MakeStoredEchoSummary(TimeAnchorRecording);
	}
	Summary.bHasPendingRecording = bHasPendingRecording;
	if (bHasPendingRecording)
	{
		Summary.PendingRecording = MakeStoredEchoSummary(PendingRecording);
	}
	Summary.bHasLatestCompletedRecording = bHasLatestCompletedRecording;
	if (bHasLatestCompletedRecording)
	{
		Summary.LatestCompletedRecording = MakeStoredEchoSummary(LatestCompletedRecording);
	}
	return Summary;
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
	const int32 Count = FMath::Clamp(RequestedCount, 0, ReEchoTimeAnchor::MaximumResolvedEchoes);
	if (Count == 0)
	{
		return Result;
	}
	if (bHasTimeAnchorRecording && CurrentBuild.CardState.Runtime.bHasAnchorRecording &&
	    TimeAnchorRecording.Id == CurrentBuild.CardState.Runtime.AnchorRecordingId)
	{
		Result.Add(TimeAnchorRecording);
		return Result;
	}

	// Without a valid G_3_02 anchor, replay the rolling previous encounters automatically.
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
	for (const FReEchoSaveSlotSummary& Summary : GetSaveSlotSummaries())
	{
		if (Summary.bOccupied)
		{
			return true;
		}
	}
	return false;
}

FString UReEchoRunSubsystem::GetSaveSlotName(const int32 SlotIndex) const
{
	return MakeRunSaveSlotName(SlotIndex);
}

FString UReEchoRunSubsystem::GetSaveSlotPreviewPath(const int32 SlotIndex) const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(),
	                       TEXT("SaveScreenshots"),
	                       FString::Printf(TEXT("ReEchoRunSlot%d.png"), SlotIndex + 1));
}

const UReEchoRunSaveGame* UReEchoRunSubsystem::LoadValidatedSaveForSlot(const int32 SlotIndex, bool& bOutLegacy) const
{
	bOutLegacy = false;
	if (SlotIndex < 0 || SlotIndex >= SaveSlotCount)
	{
		return nullptr;
	}
	const FString SlotName = GetSaveSlotName(SlotIndex);
	const UReEchoRunSaveGame* SaveGame =
	    Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, RunSaveUserIndex));
	if (!SaveGame && SlotIndex == 0 && !UGameplayStatics::DoesSaveGameExist(SlotName, RunSaveUserIndex))
	{
		SaveGame = Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromSlot(RunSaveSlot, RunSaveUserIndex));
		bOutLegacy = SaveGame != nullptr;
	}
	if (!SaveGame || !IsValidResumableSave(*SaveGame))
	{
		return nullptr;
	}
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	return Snapshot.IsValid() &&
	               ReEchoWeaponRuntime::GetBuildConfigurationError(*Snapshot, SaveGame->CurrentBuild).IsEmpty()
	           ? SaveGame
	           : nullptr;
}

TArray<FReEchoSaveSlotSummary> UReEchoRunSubsystem::GetSaveSlotSummaries() const
{
	TArray<FReEchoSaveSlotSummary> Result;
	Result.Reserve(SaveSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < SaveSlotCount; ++SlotIndex)
	{
		FReEchoSaveSlotSummary& Summary = Result.AddDefaulted_GetRef();
		Summary.SlotIndex = SlotIndex;
		bool bLegacy = false;
		const UReEchoRunSaveGame* SaveGame = LoadValidatedSaveForSlot(SlotIndex, bLegacy);
		if (!SaveGame)
		{
			continue;
		}
		Summary.bOccupied = true;
		Summary.EncounterNumber = FMath::Max(1, SaveGame->EncounterIndex + 1);
		Summary.CardCount = SaveGame->CurrentBuild.CardState.OwnedCardIds.Num();
		if (Summary.CardCount == 0)
		{
			Summary.CardCount = SaveGame->CurrentBuild.Cards.Num();
		}
		if (SaveGame->SavedAtUtcTicks > 0)
		{
			Summary.SavedAtUtc = FDateTime(SaveGame->SavedAtUtcTicks);
		}
		Summary.PreviewScreenshotPath =
		    SaveGame->PreviewScreenshotFileName.IsEmpty()
		        ? GetSaveSlotPreviewPath(SlotIndex)
		        : FPaths::Combine(FPaths::ProjectSavedDir(), SaveGame->PreviewScreenshotFileName);
		if (bLegacy && Summary.SavedAtUtc.GetTicks() == 0)
		{
			const FString LegacyPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames/ReEchoRun.sav"));
			Summary.SavedAtUtc =
			    IFileManager::Get().GetTimeStamp(*LegacyPath) - (FDateTime::Now() - FDateTime::UtcNow());
		}
	}
	return Result;
}

bool UReEchoRunSubsystem::SelectSaveSlot(const int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= SaveSlotCount)
	{
		return false;
	}
	ActiveSaveSlotIndex = SlotIndex;
	return true;
}

bool UReEchoRunSubsystem::SelectFirstEmptySaveSlot()
{
	for (const FReEchoSaveSlotSummary& Summary : GetSaveSlotSummaries())
	{
		if (!Summary.bOccupied)
		{
			ActiveSaveSlotIndex = Summary.SlotIndex;
			return true;
		}
	}
	return false;
}

bool UReEchoRunSubsystem::SaveRun(const FReEchoEncounterRuntimeState* EncounterRuntimeState) const
{
	if (ActiveSaveSlotIndex < 0 || ActiveSaveSlotIndex >= SaveSlotCount)
	{
		UE_LOG(LogReEcho, Warning, TEXT("SaveRun rejected because no active save slot is selected."));
		return false;
	}
	UReEchoRunSaveGame* SaveGame = CreateSaveSnapshot(EncounterRuntimeState);
	return SaveGame &&
	       UGameplayStatics::SaveGameToSlot(SaveGame, GetSaveSlotName(ActiveSaveSlotIndex), RunSaveUserIndex);
}

bool UReEchoRunSubsystem::LoadSavedRun()
{
	for (const FReEchoSaveSlotSummary& Summary : GetSaveSlotSummaries())
	{
		if (Summary.bOccupied)
		{
			return LoadSavedRunFromSlot(Summary.SlotIndex);
		}
	}
	return false;
}

bool UReEchoRunSubsystem::LoadSavedRunFromSlot(const int32 SlotIndex)
{
	bool bLegacy = false;
	const UReEchoRunSaveGame* SaveGame = LoadValidatedSaveForSlot(SlotIndex, bLegacy);
	if (!SaveGame || !RestoreSaveSnapshot(*SaveGame))
	{
		return false;
	}
	ActiveSaveSlotIndex = SlotIndex;
	return true;
}

void UReEchoRunSubsystem::DeleteSavedRun() const
{
	if (ActiveSaveSlotIndex < 0 || ActiveSaveSlotIndex >= SaveSlotCount)
	{
		return;
	}
	const FString SlotName = GetSaveSlotName(ActiveSaveSlotIndex);
	if (UGameplayStatics::DoesSaveGameExist(SlotName, RunSaveUserIndex))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, RunSaveUserIndex);
	}
	IFileManager::Get().Delete(*GetSaveSlotPreviewPath(ActiveSaveSlotIndex), false, true);
}

FString UReEchoRunSubsystem::GetActiveSaveSlotPreviewPath() const
{
	return ActiveSaveSlotIndex >= 0 && ActiveSaveSlotIndex < SaveSlotCount ? GetSaveSlotPreviewPath(ActiveSaveSlotIndex)
	                                                                       : FString();
}

bool UReEchoRunSubsystem::WriteActiveSaveSlotPreview(const TArray<uint8>& PngBytes) const
{
	const FString Path = GetActiveSaveSlotPreviewPath();
	if (Path.IsEmpty() || PngBytes.IsEmpty())
	{
		return false;
	}
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	return FFileHelper::SaveArrayToFile(PngBytes, *Path);
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
	SaveGame->LogicalSlotIndex = FMath::Max(0, ActiveSaveSlotIndex);
	SaveGame->SavedAtUtcTicks = FDateTime::UtcNow().GetTicks();
	SaveGame->PreviewScreenshotFileName = FPaths::Combine(
	    TEXT("SaveScreenshots"), FString::Printf(TEXT("ReEchoRunSlot%d.png"), SaveGame->LogicalSlotIndex + 1));
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
	SaveGame->RunSeed = RunSeed;
	SaveGame->TraitOfferSeed = TraitOfferSeed;
	SaveGame->PendingTraitCardOfferEncounterIndex = PendingTraitCardOfferEncounterIndex;
	SaveGame->PendingTraitCardIds = PendingTraitCardIds;
	SaveGame->PendingTraitCardOfferHistoryIds = PendingTraitCardOfferHistoryIds;
	SaveGame->PendingTraitCardRefreshUses = PendingTraitCardRefreshUses;
	SaveGame->EnemyShardDropSeed = EnemyShardDropSeed;
	SaveGame->RewardedEnemyShardDropKeys = RewardedEnemyShardDropKeys.Array();
	SaveGame->RewardedEnemyShardDropKeys.Sort();
	SaveGame->CurrentBuild = CurrentBuild;
	SaveGame->CurrentBuild.Cards.Reset();
	SaveGame->InventoryItems = InventoryItems;
	SaveGame->OwnedPartIds = OwnedPartIds;
	SaveGame->RuneAcquisitionCounts =
	    NormalizeRuneCounts(OwnedPartIds, CurrentBuild.EquippedParts, RuneAcquisitionCounts);
	SaveGame->PurchasedWeaponPartOfferIds = PurchasedWeaponPartOfferIds.Array();
	SaveGame->PurchasedWeaponPartOfferIds.Sort(FNameLexicalLess());
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
	SaveGame->WeaponRuneRefreshSequence = WeaponRuneRefreshSequence;
	SaveGame->WeaponRuneRefreshEncounterIndex = WeaponRuneRefreshEncounterIndex;
	SaveGame->WeaponRuneRefreshesUsed = WeaponRuneRefreshesUsed;
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
	SaveGame->StoredEchoes.Reset();
	if (bHasTimeAnchorRecording)
	{
		SaveGame->StoredEchoes.Add(TimeAnchorRecording);
	}
	SaveGame->SelectedReplayIds.Reset();
	SaveGame->StorageCapacity = 0;
	SaveGame->SpecificReplayLimit = 0;
	return SaveGame;
}

bool UReEchoRunSubsystem::RestoreSaveSnapshot(const UReEchoRunSaveGame& SaveGame)
{
	const FScopedTimeShardBalanceChange BalanceChange(*this);
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
			if (!Snapshot->FindEnabledWeapon(WeaponId))
			{
				UE_LOG(LogTemp,
				       Warning,
				       TEXT("Save restore rejected because owned weapon %s is missing or retired."),
				       *WeaponId.ToString());
				return false;
			}
			NormalizedOwnedWeapons.Add(WeaponId);
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
	if (RestoredStorage.bHasTimeAnchorRecording &&
	    (!MigrateBuildState(SaveGame.SaveVersion, *Snapshot, RestoredStorage.TimeAnchorRecording.BuildSnapshot) ||
	     !TryNormalizeRestoredRecording(*Snapshot, RestoredStorage.TimeAnchorRecording)))
	{
		return false;
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
	RunSeed = SaveGame.SaveVersion >= 24 && SaveGame.RunSeed != 0 ? SaveGame.RunSeed : MigrateLegacyRunSeed(SaveGame);
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
	if (SaveGame.SaveVersion == 4 && RestoredStorage.bHasTimeAnchorRecording)
	{
		CurrentBuild.CardState.Runtime.bHasAnchorRecording = true;
		CurrentBuild.CardState.Runtime.AnchorRecordingId = RestoredStorage.TimeAnchorRecording.Id;
	}
	RunDataSnapshot = Snapshot;
	InventoryItems = SaveGame.InventoryItems;
	OwnedPartIds = MoveTemp(NormalizedOwnedParts);
	RuneAcquisitionCounts =
	    NormalizeRuneCounts(OwnedPartIds, CurrentBuild.EquippedParts, SaveGame.RuneAcquisitionCounts);
	PurchasedWeaponPartOfferIds.Reset();
	OwnedWeaponIds = MoveTemp(NormalizedOwnedWeapons);
	if (SaveGame.SaveVersion >= 14)
	{
		WeaponPartShopOfferEncounterIndex = SaveGame.WeaponPartShopOfferEncounterIndex;
		WeaponPartShopOfferRefreshSequence = SaveGame.WeaponPartShopOfferRefreshSequence;
		WeaponPartShopOfferIds = SaveGame.WeaponPartShopOfferIds;
		for (const FName OfferId : WeaponPartShopOfferIds)
		{
			if (OfferId.IsNone())
			{
				continue;
			}
			if (SaveGame.SaveVersion >= 26)
			{
				if (SaveGame.PurchasedWeaponPartOfferIds.Contains(OfferId))
				{
					PurchasedWeaponPartOfferIds.Add(OfferId);
				}
				continue;
			}
			// Legacy saves did not record purchases. Conservatively consume cached offers from a held
			// rune family, without inventing copies or rerolling unrelated offers/budgets.
			FName TierId = OfferId;
			TSet<FName> Visited;
			while (!TierId.IsNone() && !Visited.Contains(TierId))
			{
				Visited.Add(TierId);
				if (OwnedPartIds.Contains(TierId))
				{
					PurchasedWeaponPartOfferIds.Add(OfferId);
					break;
				}
				const FReEchoCsvRuneUpgradeRow* Recipe = Snapshot->RuneUpgrades.Find(TierId);
				TierId = Recipe ? Recipe->ToPartId : NAME_None;
			}
		}
	}
	else
	{
		WeaponPartShopOfferEncounterIndex = INDEX_NONE;
		WeaponPartShopOfferRefreshSequence = INDEX_NONE;
		WeaponPartShopOfferIds.Reset();
	}
	if (SaveGame.SaveVersion >= 17)
	{
		WeaponRuneRefreshSequence = FMath::Max(0, SaveGame.WeaponRuneRefreshSequence);
		WeaponRuneRefreshEncounterIndex = SaveGame.WeaponRuneRefreshEncounterIndex;
		WeaponRuneRefreshesUsed = FMath::Max(0, SaveGame.WeaponRuneRefreshesUsed);
	}
	else
	{
		// Preserve the visible legacy page while granting one fresh v17 per-encounter budget.
		WeaponRuneRefreshSequence = FMath::Max(0, SaveGame.WeaponPartShopOfferRefreshSequence);
		WeaponRuneRefreshEncounterIndex = EncounterIndex;
		WeaponRuneRefreshesUsed = 0;
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
	bHasTimeAnchorRecording =
	    RestoredStorage.bHasTimeAnchorRecording && CurrentBuild.CardState.Runtime.bHasAnchorRecording &&
	    RestoredStorage.TimeAnchorRecording.Id == CurrentBuild.CardState.Runtime.AnchorRecordingId;
	TimeAnchorRecording = bHasTimeAnchorRecording ? RestoredStorage.TimeAnchorRecording : FReEchoRecording{};
	if (!bHasTimeAnchorRecording && CurrentBuild.CardState.Runtime.bHasAnchorRecording)
	{
		ClearTimeAnchorRecording();
	}
	PendingTraitCardIds.Reset();
	PendingTraitCardOfferHistoryIds.Reset();
	PendingTraitCardRefreshUses.Reset();
	PendingTraitCardOfferEncounterIndex = INDEX_NONE;
	if (SaveGame.SaveVersion >= 18 && SaveGame.SavedPhase == EReEchoRunPhase::CardChoice &&
	    SaveGame.PendingTraitCardOfferEncounterIndex == EncounterIndex && SaveGame.PendingTraitCardIds.Num() == 3 &&
	    SaveGame.PendingTraitCardRefreshUses.Num() == SaveGame.PendingTraitCardIds.Num())
	{
		const int32 FreeTier = ResolveConfiguredFreeTraitTier();
		const FReEchoCsvShopRefreshRuleRow* RefreshRule = Snapshot->ShopRefreshRules.Find(TEXT("Default"));
		const TArray<FName> RestoredOfferHistory =
		    SaveGame.SaveVersion >= 22 ? SaveGame.PendingTraitCardOfferHistoryIds : SaveGame.PendingTraitCardIds;
		TSet<FName> SeenHistoryIds;
		for (const FName CardId : RestoredOfferHistory)
		{
			const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId);
			if (!Card || !IsCardCompatibleWithPackTier(*Card, FreeTier) || SeenHistoryIds.Contains(CardId))
			{
				return false;
			}
			SeenHistoryIds.Add(CardId);
		}
		TSet<FName> SeenIds;
		for (int32 SlotIndex = 0; SlotIndex < SaveGame.PendingTraitCardIds.Num(); ++SlotIndex)
		{
			const FName CardId = SaveGame.PendingTraitCardIds[SlotIndex];
			const FReEchoCardDefinition* Card = Snapshot->CardCatalog->Find(CardId);
			const int32 Uses = SaveGame.PendingTraitCardRefreshUses[SlotIndex];
			if (!Card || !IsCardCompatibleWithPackTier(*Card, FreeTier) || SeenIds.Contains(CardId) ||
			    !SeenHistoryIds.Contains(CardId) || Uses < 0 || !RefreshRule ||
			    Uses > RefreshRule->CardSlotRefreshLimit)
			{
				return false;
			}
			SeenIds.Add(CardId);
		}
		PendingTraitCardIds = SaveGame.PendingTraitCardIds;
		PendingTraitCardOfferHistoryIds = RestoredOfferHistory;
		PendingTraitCardRefreshUses = SaveGame.PendingTraitCardRefreshUses;
		PendingTraitCardOfferEncounterIndex = EncounterIndex;
	}
	PendingEncounterResume = NormalizedEncounterRuntimeState;
	if (SaveGame.SavedPhase != EReEchoRunPhase::Encounter)
	{
		PendingEncounterResume = {};
	}
	// Reconcile inventory-driven permanent card milestones after both the saved card state and
	// normalized owned-part inventory have been restored. This also migrates older saves that
	// already own all six cores but predate the persisted completion flag.
	ReevaluateCoreCollectionCard();
	SetPhase(SaveGame.SavedPhase == EReEchoRunPhase::LegacyForgeChoice ? EReEchoRunPhase::CardChoice
	                                                                   : SaveGame.SavedPhase);
	ReEchoBuildTrace::LogSnapshot(TEXT("RunRestored"), EncounterIndex, Phase, CurrentBuild);
	return true;
}
