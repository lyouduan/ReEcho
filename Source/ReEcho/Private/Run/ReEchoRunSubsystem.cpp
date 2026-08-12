#include "Run/ReEchoRunSubsystem.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Core/ReEchoBalanceSettings.h"
#include "ReEcho.h"
#include "Run/ReEchoCharacterPromotion.h"
#include "Run/ReEchoRunSaveGame.h"
#include "Run/ReEchoShopCatalog.h"
#include "Weapons/ReEchoWeaponRuntime.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr const TCHAR* TraitOfferGroup = TEXT("Trait");
constexpr const TCHAR* ForgeOfferGroup = TEXT("Forge");

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
	InventoryItems.Reset();
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

TSharedPtr<const FReEchoCsvDataSnapshot> UReEchoRunSubsystem::GetRunDataSnapshot() const
{
	return RunDataSnapshot.IsValid() ? RunDataSnapshot : FReEchoCsvDataRegistry::GetSnapshot();
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
		// A failed encounter never becomes storage eligible and never touches the rolling latest echo.
		SetPhase(EReEchoRunPhase::Failed);
		return;
	}
	StagePendingRecording(Recording);
	TimeShards += 15;
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

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvCardRow* Card = Snapshot.IsValid() ? Snapshot->FindCard(CardId) : nullptr;
	if (!Card || !Card->bEnabled)
	{
		return false;
	}

	const FName SageBonusChoiceFlag = TEXT("SageBonusChoice");
	const FName NormalTraitSelectionsFlag = TEXT("NormalTraitSelections");
	const bool bSageBonusChoice = CurrentBuild.RuleFlags.Contains(SageBonusChoiceFlag);
	bool bSageBonus = false;
	FReEchoBuildSnapshot PendingBuild;
	if (!TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        if (!ApplyCardEffects(*Card, BaseBuild))
		        {
			        return false;
		        }
		        BaseBuild.Cards.Add(CardId);
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
	PendingTraitCardIds.Reset();
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

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	const FReEchoCsvCardRow* Card = Snapshot.IsValid() ? Snapshot->FindCard(ForgeId) : nullptr;
	FReEchoBuildSnapshot PendingBuild;
	if (!Card || !Card->bEnabled || Card->OfferGroup != ForgeOfferGroup ||
	    !TryMutateAuthoritativeBuild(
	        *Snapshot,
	        CurrentBuild,
	        [&](FReEchoBuildSnapshot& BaseBuild)
	        {
		        if (!ApplyCardEffects(*Card, BaseBuild))
		        {
			        return false;
		        }
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

	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = GetRunDataSnapshot();
	FReEchoBuildSnapshot PendingBuild;
	if (!Snapshot.IsValid() || !TryMutateAuthoritativeBuild(
	                               *Snapshot,
	                               CurrentBuild,
	                               [&](FReEchoBuildSnapshot& BaseBuild)
	                               {
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
		                               return true;
	                               },
	                               PendingBuild))
	{
		return false;
	}
	TimeShards -= Offer->Price;
	InventoryItems.Add(ItemId);
	CurrentBuild = PendingBuild;
	return true;
}

void UReEchoRunSubsystem::ResetEchoStorage()
{
	bHasPendingRecording = false;
	PendingRecording = {};
	bHasLatestCompletedRecording = false;
	LatestCompletedRecording = {};
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
	const int32 Count = FMath::Clamp(RequestedCount, 0, ReEchoEchoStorage::MaxStorageCapacity);
	if (Count == 0)
	{
		return Result;
	}

	const int32 AllowedCount = FMath::Clamp(SpecificReplayLimit, 0, ReEchoEchoStorage::MaxSpecificReplayLimit);

	// Once the specific replay ability is unlocked, only explicitly selected stored echoes are
	// replayed. An empty or stale selection resolves to no echo on purpose; we must never fall
	// back to the rolling latest, because that would silently restore implicit latest behavior.
	if (AllowedCount > 0)
	{
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
	if (bHasLatestCompletedRecording)
	{
		Result.Add(LatestCompletedRecording);
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
	SaveGame->CurrentBuild = CurrentBuild;
	SaveGame->InventoryItems = InventoryItems;
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
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!Snapshot.IsValid())
	{
		return false;
	}
	FReEchoBuildSnapshot NormalizedCurrentBuild;
	if (!ReEchoWeaponRuntime::GetBuildConfigurationError(*Snapshot, SaveGame.CurrentBuild).IsEmpty() ||
	    !TryNormalizeEquipmentBuild(*Snapshot, SaveGame.CurrentBuild, NormalizedCurrentBuild))
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
	    !TryNormalizeRestoredRecording(*Snapshot, RestoredStorage.PendingRecording))
	{
		return false;
	}
	if (RestoredStorage.bHasLatestCompletedRecording &&
	    !TryNormalizeRestoredRecording(*Snapshot, RestoredStorage.LatestCompletedRecording))
	{
		return false;
	}
	for (FReEchoRecording& Stored : RestoredStorage.StoredEchoes)
	{
		if (!TryNormalizeRestoredRecording(*Snapshot, Stored))
		{
			return false;
		}
	}
	FReEchoEncounterRuntimeState NormalizedEncounterRuntimeState = SaveGame.EncounterRuntimeState;
	if (SaveGame.EncounterRuntimeState.bValid)
	{
		FString ActiveRecordingError;
		if (!ValidateRecordingAgainstSnapshot(
		        *Snapshot, SaveGame.EncounterRuntimeState.ActiveRecording, ActiveRecordingError))
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
	CurrentBuild = NormalizedCurrentBuild;
	RunDataSnapshot = Snapshot;
	InventoryItems = SaveGame.InventoryItems;
	bAutomaticAttackMode = SaveGame.SaveVersion >= 6 ? SaveGame.bAutomaticAttackMode : true;
	bHasPendingRecording = RestoredStorage.bHasPendingRecording;
	PendingRecording = RestoredStorage.bHasPendingRecording ? RestoredStorage.PendingRecording : FReEchoRecording{};
	bHasLatestCompletedRecording = RestoredStorage.bHasLatestCompletedRecording;
	LatestCompletedRecording =
	    RestoredStorage.bHasLatestCompletedRecording ? RestoredStorage.LatestCompletedRecording : FReEchoRecording{};
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
