#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ReEchoGameMode.generated.h"
class AReEchoEncounterDirector;
class AReEchoEchoActor;
class AReEchoPlayerPawn;
class UReEchoRestartWidget;
class UReEchoTraitCardChoiceWidget;
UCLASS()

class REECHO_API AReEchoGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AReEchoGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	void TogglePauseMenu();

private:
	UPROPERTY()
	TObjectPtr<AReEchoEncounterDirector> Director;
	UPROPERTY()
	TObjectPtr<AReEchoPlayerPawn> Player;
	UPROPERTY()
	TArray<TObjectPtr<AReEchoEchoActor>> Echoes;

	UPROPERTY()
	TObjectPtr<UReEchoRestartWidget> RestartWidget;
	bool bRestartScreenIsDeath = false;

	UPROPERTY()
	TObjectPtr<UReEchoTraitCardChoiceWidget> TraitCardChoiceWidget;
	bool bEncounterTransitioning = false;
	UFUNCTION()
	void HandleFixedStep(float FixedDeltaSeconds);
	UFUNCTION()
	void HandleEncounterEnded();
	UFUNCTION()
	void HandlePlayerSkill(FVector Position, FName SkillId);

	UFUNCTION()
	void HandlePlayerWeaponChanged(FName WeaponId);
	UFUNCTION()
	void HandlePlayerDeath();

	UFUNCTION()
	void HandleRestartRequested();

	UFUNCTION()
	void HandleResumeRequested();

	UFUNCTION()
	void HandleQuitRequested();

	UFUNCTION()
	void HandleTraitCardSelected(FName CardId);
	void CreateArena();
	void BeginNextEncounter();
	void SpawnEnemies(int32 EncounterIndex);
	void ClearCombatants();
	void ShowRestartScreen(bool bDeathScreen = true);
	void ShowTraitCardChoice();
	void RestoreGameInput();
};
