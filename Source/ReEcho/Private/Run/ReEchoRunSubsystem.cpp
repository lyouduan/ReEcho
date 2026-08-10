#include "Run/ReEchoRunSubsystem.h"

#include "Core/ReEchoBalanceSettings.h"
#include "Run/ReEchoCharacterPromotion.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoShopCatalog.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const FString RunSaveSlot = TEXT("ReEchoRun");
constexpr int32 RunSaveUserIndex = 0;

bool IsValidResumableSave(const UReEchoRunSaveGame& SaveGame)
{
	if (SaveGame.SaveVersion != UReEchoRunSaveGame::CurrentSaveVersion ||
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
	PendingEncounterResume = {};
	CurrentBuild = {};
	CurrentBuild.CharacterId = CharacterId;
	CurrentBuild.WeaponId = WeaponId;
	CurrentBuild.Stats.HpPoint = 15.0f;
	CurrentBuild.Stats.HpMax = 15.0f;
	CurrentBuild.Stats.PhysicalAttack = 5.0f;
	CurrentBuild.Stats.ElementalAttack = 5.0f;
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
	if (ReEchoCharacterPromotion::IsRole(CurrentBuild, TEXT("Poet")))
	{
		CurrentBuild.Stats.ReactionEfficiency += 0.05f;
	}
	if (EncounterIndex >= GetDefault<UReEchoBalanceSettings>()->GetTotalEncounterCount())
	{
		SetPhase(bBossKilled ? EReEchoRunPhase::Summary : EReEchoRunPhase::Failed);
		return;
	}
	SetPhase(ReEchoCharacterPromotion::IsRole(CurrentBuild, TEXT("Brave")) ? EReEchoRunPhase::ForgeChoice
	                                                                       : EReEchoRunPhase::CardChoice);
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

	TArray<FReEchoTraitCardOffer> Offers = {
	    MakeTraitOffer(TEXT("FORGE_LIGHT"),
	                   NSLOCTEXT("ReEcho", "ForgeLight", "轻度锻炼"),
	                   NSLOCTEXT("ReEcho", "ForgeLightDesc", "生命 +2，物理与元素攻击 +1")),
	    MakeTraitOffer(TEXT("FORGE_MEDIUM"),
	                   NSLOCTEXT("ReEcho", "ForgeMedium", "中度锻炼"),
	                   NSLOCTEXT("ReEcho", "ForgeMediumDesc", "生命 -2，物理与元素攻击 +2")),
	    MakeTraitOffer(TEXT("FORGE_EXTREME"),
	                   NSLOCTEXT("ReEcho", "ForgeExtreme", "极限锻炼"),
	                   NSLOCTEXT("ReEcho", "ForgeExtremeDesc", "生命 -6，物理与元素攻击 +2"))};
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

	if (ForgeId == TEXT("FORGE_LIGHT"))
	{
		CurrentBuild.Stats.HpMax += 2.0f;
		CurrentBuild.Stats.PhysicalAttack += 1.0f;
		CurrentBuild.Stats.ElementalAttack += 1.0f;
	}
	else if (ForgeId == TEXT("FORGE_MEDIUM"))
	{
		CurrentBuild.Stats.HpMax -= 2.0f;
		CurrentBuild.Stats.PhysicalAttack += 2.0f;
		CurrentBuild.Stats.ElementalAttack += 2.0f;
	}
	else if (ForgeId == TEXT("FORGE_EXTREME"))
	{
		CurrentBuild.Stats.HpMax -= 6.0f;
		CurrentBuild.Stats.PhysicalAttack += 2.0f;
		CurrentBuild.Stats.ElementalAttack += 2.0f;
	}
	else
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

bool UReEchoRunSubsystem::HasSavedRun() const
{
	const UReEchoRunSaveGame* SaveGame =
	    Cast<UReEchoRunSaveGame>(UGameplayStatics::LoadGameFromSlot(RunSaveSlot, RunSaveUserIndex));
	return SaveGame && IsValidResumableSave(*SaveGame);
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
	SaveGame->CurrentBuild = CurrentBuild;
	SaveGame->InventoryItems = InventoryItems;
	SaveGame->RecordingHistory = RecordingHistory;
	SaveGame->AnchorId = AnchorId;
	return SaveGame;
}

bool UReEchoRunSubsystem::RestoreSaveSnapshot(const UReEchoRunSaveGame& SaveGame)
{
	if (!IsValidResumableSave(SaveGame))
	{
		return false;
	}

	EncounterIndex = FMath::Max(0, SaveGame.EncounterIndex);
	TimeShards = FMath::Max(0, SaveGame.TimeShards);
	CurrentBuild = SaveGame.CurrentBuild;
	InventoryItems = SaveGame.InventoryItems;
	RecordingHistory = SaveGame.RecordingHistory;
	AnchorId = SaveGame.AnchorId;
	PendingTraitCardIds.Reset();
	PendingEncounterResume = SaveGame.EncounterRuntimeState;
	if (SaveGame.SavedPhase != EReEchoRunPhase::Encounter)
	{
		PendingEncounterResume = {};
	}
	SetPhase(SaveGame.SavedPhase);
	return true;
}
