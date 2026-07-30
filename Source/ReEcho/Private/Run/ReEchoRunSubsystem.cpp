#include "Run/ReEchoRunSubsystem.h"

#include "Core/ReEchoBalanceSettings.h"
#include "Run/ReEchoShopCatalog.h"

namespace
{
FReEchoTraitCardOffer MakeTraitOffer(const TCHAR* CardId, const FText& DisplayName, const FText& Description)
{
	FReEchoTraitCardOffer Offer;
	Offer.CardId = FName(CardId);
	Offer.DisplayName = DisplayName;
	Offer.Description = Description;
	return Offer;
}

const TArray<FReEchoTraitCardOffer>& GetTraitCatalog()
{
	static const TArray<FReEchoTraitCardOffer> Catalog = {
	    MakeTraitOffer(TEXT("G_1_01"),
	                   NSLOCTEXT("ReEcho", "SpeedTraitName", "时序加速"),
	                   NSLOCTEXT("ReEcho", "SpeedTraitDescription", "移动速度 +10%，攻击速度 +10%")),
	    MakeTraitOffer(TEXT("G_1_02"),
	                   NSLOCTEXT("ReEcho", "HealthTraitName", "生命延展"),
	                   NSLOCTEXT("ReEcho", "HealthTraitDescription", "最大生命 +15，立即恢复等量生命")),
	    MakeTraitOffer(TEXT("G_1_03"),
	                   NSLOCTEXT("ReEcho", "BlockTraitName", "应急格挡"),
	                   NSLOCTEXT("ReEcho", "BlockTraitDescription", "每个关卡抵挡 1 次伤害")),
	    MakeTraitOffer(TEXT("G_1_04"),
	                   NSLOCTEXT("ReEcho", "PhysicalTraitName", "物理增幅"),
	                   NSLOCTEXT("ReEcho", "PhysicalTraitDescription", "物理攻击 +2")),
	    MakeTraitOffer(TEXT("G_1_05"),
	                   NSLOCTEXT("ReEcho", "ElementalTraitName", "元素增幅"),
	                   NSLOCTEXT("ReEcho", "ElementalTraitDescription", "元素攻击 +2")),
	    MakeTraitOffer(TEXT("G_1_08"),
	                   NSLOCTEXT("ReEcho", "EchoTraitName", "回响共振"),
	                   NSLOCTEXT("ReEcho", "EchoTraitDescription", "回响伤害效率 +10%"))};
	return Catalog;
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

void ShuffleOffers(TArray<FReEchoTraitCardOffer>& Offers, FRandomStream& Random)
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
	CurrentBuild.CharacterId = CharacterId;
	CurrentBuild.WeaponId = WeaponId;
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
	if (EncounterIndex >= GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount())
	{
		SetPhase(bBossKilled ? EReEchoRunPhase::Summary : EReEchoRunPhase::Failed);
		return;
	}
	SetPhase(EReEchoRunPhase::CardChoice);
}

TArray<FReEchoTraitCardOffer> UReEchoRunSubsystem::GenerateTraitCardOffers(const int32 RequestedCount)
{
	PendingTraitCardIds.Reset();
	const int32 OfferCount = FMath::Clamp(RequestedCount, 0, GetTraitCatalog().Num());
	if (OfferCount == 0 || Phase != EReEchoRunPhase::CardChoice)
	{
		return {};
	}

	FRandomStream Random(BuildTraitOfferSeed(EncounterIndex, CurrentBuild.Cards));
	TArray<FReEchoTraitCardOffer> Result;
	int32 StackLevel = 0;
	while (Result.Num() < OfferCount)
	{
		TArray<FReEchoTraitCardOffer> StackBucket;
		for (const FReEchoTraitCardOffer& Offer : GetTraitCatalog())
		{
			if (GetTraitStackCount(CurrentBuild.Cards, Offer.CardId) == StackLevel)
			{
				StackBucket.Add(Offer);
			}
		}

		ShuffleOffers(StackBucket, Random);
		for (const FReEchoTraitCardOffer& Offer : StackBucket)
		{
			if (Result.Num() >= OfferCount)
			{
				break;
			}
			Result.Add(Offer);
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

	if (CardId == TEXT("G_1_01"))
	{
		CurrentBuild.Stats.MovementSpeed += 0.10f;
		CurrentBuild.Stats.AttackSpeed += 0.10f;
	}
	else if (CardId == TEXT("G_1_02"))
	{
		CurrentBuild.Stats.HpMax += 15.0f;
	}
	else if (CardId == TEXT("G_1_03"))
	{
		CurrentBuild.Stats.Block += 1;
	}
	else if (CardId == TEXT("G_1_04"))
	{
		CurrentBuild.Stats.PhysicalAttack += 2.0f;
	}
	else if (CardId == TEXT("G_1_05"))
	{
		CurrentBuild.Stats.ElementalAttack += 2.0f;
	}
	else if (CardId == TEXT("G_1_08"))
	{
		CurrentBuild.Stats.EchoEfficiency += 0.10f;
	}
	else
	{
		return false;
	}

	CurrentBuild.Cards.Add(CardId);
	PendingTraitCardIds.Reset();
	SetPhase(EReEchoRunPhase::Planning);
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
