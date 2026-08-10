#include "Run/ReEchoRunSubsystem.h"

#include "Core/ReEchoBalanceSettings.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Run/ReEchoCharacterPromotion.h"
#include "Run/ReEchoShopCatalog.h"

namespace
{
constexpr const TCHAR* TraitOfferGroup = TEXT("Trait");
constexpr const TCHAR* ForgeOfferGroup = TEXT("Forge");

FReEchoTraitCardOffer MakeTraitOffer(const FReEchoCsvCardRow& Card)
{
	FReEchoTraitCardOffer Offer;
	Offer.CardId = Card.Id;
	Offer.DisplayName = FText::FromString(Card.DisplayName);
	Offer.Description = FText::FromString(Card.Description);
	return Offer;
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

void ShuffleOffers(TArray<FReEchoCsvCardRow>& Offers, FRandomStream& Random)
{
	for (int32 Index = Offers.Num() - 1; Index > 0; --Index)
	{
		Offers.Swap(Index, Random.RandRange(0, Index));
	}
}

int32 BuildTraitOfferSeed(const int32 EncounterIndex, const TArray<FName>& OwnedCards)
{
	uint32 Seed = HashCombine(GetTypeHash(EncounterIndex), GetTypeHash(OwnedCards.Num()));
	for (const FName CardId : OwnedCards)
	{
		Seed = HashCombine(Seed, GetTypeHash(CardId));
	}
	return static_cast<int32>(Seed);
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

bool CurrentCharacterHasPassive(const FReEchoBuildSnapshot& Build, const FName PassiveBehaviorId)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvCharacterRow* Character = Snapshot.IsValid() ? Snapshot->FindCharacter(Build.CharacterId) : nullptr;
	return Character && Character->PassiveBehaviorId == PassiveBehaviorId;
}
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
	InventoryItems.Reset();
	RecordingHistory.Reset();
	AnchorId.Invalidate();
	PendingTraitCardIds.Reset();
	CurrentBuild = {};
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvCharacterRow* Character = Snapshot.IsValid() ? Snapshot->FindCharacter(CharacterId) : nullptr;
	if (Character && Character->bEnabled)
	{
		CurrentBuild.CharacterId = Character->Id;
		CurrentBuild.WeaponId = WeaponId.IsNone() ? Character->DefaultWeaponId : WeaponId;
		CurrentBuild.Stats = Character->BaseStats;
		CurrentBuild.Stats.RoleId = Character->RoleId == TEXT("None") ? NAME_None : Character->RoleId;
		CurrentBuild.RuleFlags.Add(TEXT("BaseCharacterId"), Character->Id.ToString());
	}
	else
	{
		CurrentBuild.CharacterId = CharacterId;
		CurrentBuild.WeaponId = WeaponId;
		CurrentBuild.Stats.HpPoint = 15.0f;
		CurrentBuild.Stats.HpMax = 15.0f;
		CurrentBuild.Stats.PhysicalAttack = 5.0f;
		CurrentBuild.Stats.ElementalAttack = 5.0f;
	}
	SetPhase(EReEchoRunPhase::Planning);
}

void UReEchoRunSubsystem::SetEquippedWeapon(const FName WeaponId)
{
	CurrentBuild.WeaponId = WeaponId;
}

void UReEchoRunSubsystem::BeginEncounter()
{
	++EncounterIndex;
	SetPhase(EReEchoRunPhase::Encounter);
}

void UReEchoRunSubsystem::CompleteEncounter(const FReEchoRecording& Recording,
                                            const bool bPlayerSurvived,
                                            const bool bBossKilled)
{
	if (!bPlayerSurvived)
	{
		SetPhase(EReEchoRunPhase::Failed);
		return;
	}
	AddRecording(Recording);
	TimeShards += 15;
	if (CurrentCharacterHasPassive(CurrentBuild, TEXT("Character.PoetReactionGrowth")))
	{
		const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
		const FReEchoCsvCharacterRow* Character =
		    Snapshot.IsValid() ? Snapshot->FindCharacter(CurrentBuild.CharacterId) : nullptr;
		CurrentBuild.Stats.ReactionEfficiency += Character ? Character->PassiveValue : 0.05f;
	}
	if (EncounterIndex >= GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount())
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
	const TArray<FReEchoCsvCardRow> Catalog = GetOfferCatalog(TraitOfferGroup);
	const int32 OfferCount = FMath::Clamp(RequestedCount, 0, Catalog.Num());
	if (OfferCount == 0 || Phase != EReEchoRunPhase::CardChoice)
	{
		return {};
	}

	FRandomStream Random(BuildTraitOfferSeed(EncounterIndex, CurrentBuild.Cards));
	TArray<FReEchoTraitCardOffer> Result;
	int32 StackLevel = 0;
	while (Result.Num() < OfferCount)
	{
		TArray<FReEchoCsvCardRow> StackBucket;
		for (const FReEchoCsvCardRow& Card : Catalog)
		{
			if (GetTraitStackCount(CurrentBuild.Cards, Card.Id) == StackLevel)
			{
				StackBucket.Add(Card);
			}
		}

		ShuffleOffers(StackBucket, Random);
		for (const FReEchoCsvCardRow& Card : StackBucket)
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

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvCardRow* Card = Snapshot.IsValid() ? Snapshot->FindCard(CardId) : nullptr;
	if (!Card || !Card->bEnabled || !ApplyCardEffects(*Card, CurrentBuild))
	{
		return false;
	}

	const FName SageBonusChoiceFlag = TEXT("SageBonusChoice");
	const FName NormalTraitSelectionsFlag = TEXT("NormalTraitSelections");
	const bool bSageBonusChoice = CurrentBuild.RuleFlags.Contains(SageBonusChoiceFlag);

	CurrentBuild.Cards.Add(CardId);
	ReEchoCharacterPromotion::TryPromote(CurrentBuild);
	PendingTraitCardIds.Reset();
	if (bSageBonusChoice)
	{
		CurrentBuild.RuleFlags.Remove(SageBonusChoiceFlag);
		SetPhase(EReEchoRunPhase::Planning);
		return true;
	}

	const int32 NormalTraitSelections = FCString::Atoi(*CurrentBuild.RuleFlags.FindRef(NormalTraitSelectionsFlag)) + 1;
	CurrentBuild.RuleFlags.Add(NormalTraitSelectionsFlag, FString::FromInt(NormalTraitSelections));
	const bool bSageBonus =
	    ReEchoCharacterPromotion::IsRole(CurrentBuild, TEXT("Sage")) && NormalTraitSelections % 4 == 0;
	if (bSageBonus)
	{
		CurrentBuild.RuleFlags.Add(SageBonusChoiceFlag, TEXT("1"));
	}
	SetPhase(bSageBonus ? EReEchoRunPhase::CardChoice : EReEchoRunPhase::Planning);
	return true;
}

TArray<FReEchoTraitCardOffer> UReEchoRunSubsystem::GenerateForgeOffers()
{
	PendingTraitCardIds.Reset();
	if (Phase != EReEchoRunPhase::ForgeChoice)
	{
		return {};
	}

	TArray<FReEchoTraitCardOffer> Offers;
	for (const FReEchoCsvCardRow& Card : GetOfferCatalog(ForgeOfferGroup))
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

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	const FReEchoCsvCardRow* Card = Snapshot.IsValid() ? Snapshot->FindCard(ForgeId) : nullptr;
	if (!Card || !Card->bEnabled || Card->OfferGroup != ForgeOfferGroup || !ApplyCardEffects(*Card, CurrentBuild))
	{
		return false;
	}

	CurrentBuild.Stats.HpMax = FMath::Max(1.0f, CurrentBuild.Stats.HpMax);
	PendingTraitCardIds.Reset();
	SetPhase(EReEchoRunPhase::CardChoice);
	return true;
}

bool UReEchoRunSubsystem::PurchaseShopItem(const FName ItemId)
{
	if (InventoryItems.Contains(ItemId))
	{
		return false;
	}

	const FReEchoShopOffer* Offer = GetReEchoShopCatalog().FindByPredicate(
	    [&](const FReEchoShopOffer& Candidate)
	    {
		    return Candidate.ItemId == ItemId;
	    });
	if (!Offer || TimeShards < Offer->Price)
	{
		return false;
	}

	TimeShards -= Offer->Price;
	InventoryItems.Add(ItemId);
	if (ItemId == TEXT("SHOP_RUSTED_SCISSORS"))
	{
		CurrentBuild.Stats.PhysicalAttack += 2.0f;
	}
	else if (ItemId == TEXT("SHOP_DREAM_FRUIT"))
	{
		CurrentBuild.Stats.HpMax += 10.0f;
	}
	else if (ItemId == TEXT("SHOP_BLACK_FEATHER"))
	{
		CurrentBuild.Stats.MovementSpeed += 0.1f;
	}
	else if (ItemId == TEXT("SHOP_OLD_COIN"))
	{
		CurrentBuild.Stats.EchoEfficiency += 0.1f;
	}
	return true;
}

void UReEchoRunSubsystem::AddRecording(const FReEchoRecording& Recording)
{
	RecordingHistory.Insert(Recording, 0);
	RecordingHistory.SetNum(
	    FMath::Min(RecordingHistory.Num(), GetDefault<UReEchoBalanceSettings>()->RecordingHistoryLimit));
}

void UReEchoRunSubsystem::SetAnchor(const FGuid RecordingId)
{
	if (RecordingHistory.ContainsByPredicate(
	        [&](const FReEchoRecording& R)
	        {
		        return R.Id == RecordingId;
	        }))
	{
		AnchorId = RecordingId;
	}
}

void UReEchoRunSubsystem::ClearAnchor()
{
	AnchorId.Invalidate();
}

TArray<FReEchoRecording> UReEchoRunSubsystem::GetEchoRecordings(const int32 RequestedCount) const
{
	TArray<FReEchoRecording> Result;
	const int32 Count = FMath::Clamp(RequestedCount, 0, 3);
	if (Count == 0)
	{
		return Result;
	}
	if (AnchorId.IsValid())
	{
		if (const FReEchoRecording* Anchor = RecordingHistory.FindByPredicate(
		        [&](const FReEchoRecording& R)
		        {
			        return R.Id == AnchorId;
		        }))
		{
			Result.Add(*Anchor);
		}
	}
	for (const FReEchoRecording& Candidate : RecordingHistory)
	{
		if (Result.Num() >= Count)
		{
			break;
		}
		if (Candidate.Id != AnchorId)
		{
			Result.Add(Candidate);
		}
	}
	return Result;
}

bool UReEchoRunSubsystem::ShouldOpenShopAfterCurrentEncounter() const
{
	return EncounterIndex == 2 || EncounterIndex == 4;
}
