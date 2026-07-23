#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoRunSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReEchoRunPhaseChanged, EReEchoRunPhase, NewPhase);

UCLASS()
class REECHO_API UReEchoRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoRunPhaseChanged OnPhaseChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EReEchoRunPhase Phase = EReEchoRunPhase::CharacterSelect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 EncounterIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 TimeShards = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FReEchoBuildSnapshot CurrentBuild;

	UFUNCTION(BlueprintCallable)
	void StartRun(FName CharacterId, FName WeaponId);

	UFUNCTION(BlueprintCallable)
	void BeginEncounter();

	UFUNCTION(BlueprintCallable)
	void SetEquippedWeapon(FName WeaponId);
	UFUNCTION(BlueprintCallable)
	void CompleteEncounter(const FReEchoRecording& Recording, bool bPlayerSurvived, bool bBossKilled);

	UFUNCTION(BlueprintCallable)
	TArray<FReEchoTraitCardOffer> GenerateTraitCardOffers(int32 RequestedCount) const;

	UFUNCTION(BlueprintCallable)
	bool ApplyTraitCard(FName CardId);
	UFUNCTION(BlueprintCallable)
	void AddRecording(const FReEchoRecording& Recording);

	UFUNCTION(BlueprintCallable)
	void SetAnchor(FGuid RecordingId);

	UFUNCTION(BlueprintCallable)
	void ClearAnchor();

	UFUNCTION(BlueprintPure)
	TArray<FReEchoRecording> GetEchoRecordings(int32 RequestedCount) const;

	UFUNCTION(BlueprintPure)
	bool ShouldOpenShopAfterCurrentEncounter() const;

private:
	UPROPERTY()
	TArray<FReEchoRecording> RecordingHistory;

	UPROPERTY()
	FGuid AnchorId;

	void SetPhase(EReEchoRunPhase NewPhase);
};
