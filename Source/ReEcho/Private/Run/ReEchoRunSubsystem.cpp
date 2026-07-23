#include "Run/ReEchoRunSubsystem.h"

#include "Core/ReEchoBalanceSettings.h"
#include "Algo/RandomShuffle.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const TSet<FName>& GetSupportedTraitCardIds()
{
	static const TSet<FName> SupportedTraitCardIds = {
	    TEXT("G_1_01"), TEXT("G_1_02"), TEXT("G_1_03"), TEXT("G_1_04"), TEXT("G_1_05"), TEXT("G_1_08")};
	return SupportedTraitCardIds;
}

bool LoadTraitCardObjects(TArray<TSharedPtr<FJsonValue>>& OutCards)
{
	FString JsonText;
	const FString CardsPath = FPaths::ProjectContentDir() / TEXT("Data/cards.json");
	if (!FFileHelper::LoadFileToString(JsonText, *CardsPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to load trait cards from %s"), *CardsPath);
		return false;
	}

	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, OutCards))
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to parse trait cards from %s"), *CardsPath);
		return false;
	}

	return true;
}

FText BuildTraitDescription(const TSharedPtr<FJsonObject>& Effect)
{
	double Value = 0.0;
	if (Effect->TryGetNumberField(TEXT("hpMaxAdd"), Value))
	{
		return FText::Format(NSLOCTEXT("ReEcho", "HealthTraitDescription", "最大生命 +{0}，立即恢复等量生命"),
		                     FText::AsNumber(FMath::RoundToInt(Value)));
	}
	if (Effect->TryGetNumberField(TEXT("pAtkAdd"), Value))
	{
		return FText::Format(NSLOCTEXT("ReEcho", "PhysicalTraitDescription", "物理攻击 +{0}"),
		                     FText::AsNumber(FMath::RoundToInt(Value)));
	}
	if (Effect->TryGetNumberField(TEXT("eAtkAdd"), Value))
	{
		return FText::Format(NSLOCTEXT("ReEcho", "ElementalTraitDescription", "元素攻击 +{0}"),
		                     FText::AsNumber(FMath::RoundToInt(Value)));
	}
	if (Effect->TryGetNumberField(TEXT("blockPerEncounter"), Value))
	{
		return FText::Format(NSLOCTEXT("ReEcho", "BlockTraitDescription", "每个关卡抵挡 {0} 次伤害"),
		                     FText::AsNumber(FMath::RoundToInt(Value)));
	}
	if (Effect->TryGetNumberField(TEXT("echoEfficiencyAdd"), Value))
	{
		return FText::Format(NSLOCTEXT("ReEcho", "EchoTraitDescription", "回响伤害效率 +{0}%"),
		                     FText::AsNumber(FMath::RoundToInt(Value * 100.0)));
	}

	double MoveSpeed = 0.0;
	double AttackSpeed = 0.0;
	if (Effect->TryGetNumberField(TEXT("moveSpeedAdd"), MoveSpeed) &&
	    Effect->TryGetNumberField(TEXT("attackSpeedAdd"), AttackSpeed))
	{
		return FText::Format(NSLOCTEXT("ReEcho", "SpeedTraitDescription", "移动速度 +{0}%，攻击速度 +{1}%"),
		                     FText::AsNumber(FMath::RoundToInt(MoveSpeed * 100.0)),
		                     FText::AsNumber(FMath::RoundToInt(AttackSpeed * 100.0)));
	}

	return NSLOCTEXT("ReEcho", "UnknownTraitDescription", "强化当前时间线");
}

void AddEffectNumber(const TSharedPtr<FJsonObject>& Effect, const TCHAR* FieldName, float& Target)
{
	double Value = 0.0;
	if (Effect->TryGetNumberField(FieldName, Value))
	{
		Target += static_cast<float>(Value);
	}
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
	RecordingHistory.Reset();
	AnchorId.Invalidate();
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
	if (EncounterIndex >= 6)
	{
		SetPhase(EReEchoRunPhase::Summary);
		return;
	}
	SetPhase(EReEchoRunPhase::CardChoice);
}

TArray<FReEchoTraitCardOffer> UReEchoRunSubsystem::GenerateTraitCardOffers(const int32 RequestedCount) const
{
	TArray<TSharedPtr<FJsonValue>> CardValues;
	if (!LoadTraitCardObjects(CardValues))
	{
		return {};
	}

	TArray<FReEchoTraitCardOffer> AvailableCards;
	for (const TSharedPtr<FJsonValue>& CardValue : CardValues)
	{
		const TSharedPtr<FJsonObject> CardObject = CardValue->AsObject();
		if (!CardObject)
		{
			continue;
		}

		FString CardIdString;
		if (!CardObject->TryGetStringField(TEXT("id"), CardIdString))
		{
			continue;
		}

		const FName CardId(CardIdString);
		if (!GetSupportedTraitCardIds().Contains(CardId))
		{
			continue;
		}

		bool bReserved = false;
		bool bOfferable = true;
		CardObject->TryGetBoolField(TEXT("reserved"), bReserved);
		CardObject->TryGetBoolField(TEXT("offerable"), bOfferable);
		if (bReserved || !bOfferable)
		{
			continue;
		}

		FString DisplayName;
		CardObject->TryGetStringField(TEXT("name"), DisplayName);
		const TSharedPtr<FJsonObject>* Effect = nullptr;
		if (!CardObject->TryGetObjectField(TEXT("effect"), Effect) || !Effect)
		{
			continue;
		}

		FReEchoTraitCardOffer& Offer = AvailableCards.AddDefaulted_GetRef();
		Offer.CardId = CardId;
		Offer.DisplayName = FText::FromString(DisplayName);
		Offer.Description = BuildTraitDescription(*Effect);
	}

	Algo::RandomShuffle(AvailableCards);
	AvailableCards.SetNum(FMath::Min(RequestedCount, AvailableCards.Num()));
	return AvailableCards;
}

bool UReEchoRunSubsystem::ApplyTraitCard(const FName CardId)
{
	if (!GetSupportedTraitCardIds().Contains(CardId))
	{
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> CardValues;
	if (!LoadTraitCardObjects(CardValues))
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& CardValue : CardValues)
	{
		const TSharedPtr<FJsonObject> CardObject = CardValue->AsObject();
		if (!CardObject)
		{
			continue;
		}

		FString CandidateId;
		if (!CardObject->TryGetStringField(TEXT("id"), CandidateId) || FName(CandidateId) != CardId)
		{
			continue;
		}

		const TSharedPtr<FJsonObject>* Effect = nullptr;
		if (!CardObject->TryGetObjectField(TEXT("effect"), Effect) || !Effect)
		{
			return false;
		}

		AddEffectNumber(*Effect, TEXT("hpMaxAdd"), CurrentBuild.Stats.HpMax);
		AddEffectNumber(*Effect, TEXT("pAtkAdd"), CurrentBuild.Stats.PhysicalAttack);
		AddEffectNumber(*Effect, TEXT("eAtkAdd"), CurrentBuild.Stats.ElementalAttack);
		AddEffectNumber(*Effect, TEXT("moveSpeedAdd"), CurrentBuild.Stats.MovementSpeed);
		AddEffectNumber(*Effect, TEXT("attackSpeedAdd"), CurrentBuild.Stats.AttackSpeed);
		AddEffectNumber(*Effect, TEXT("echoEfficiencyAdd"), CurrentBuild.Stats.EchoEfficiency);

		double BlockPerEncounter = 0.0;
		if ((*Effect)->TryGetNumberField(TEXT("blockPerEncounter"), BlockPerEncounter))
		{
			CurrentBuild.Stats.Block += FMath::RoundToInt(BlockPerEncounter);
		}

		CurrentBuild.Cards.Add(CardId);
		SetPhase(EReEchoRunPhase::Planning);
		return true;
	}

	return false;
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
