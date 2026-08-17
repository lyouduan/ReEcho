#pragma once
#include "CoreMinimal.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "GameFramework/GameModeBase.h"
#include "ReEchoGameMode.generated.h"
class ACameraActor;
class AReEchoArenaCameraActor;
class AReEchoArenaSceneActor;
class AReEchoEncounterDirector;
class AReEchoEchoActor;
class AReEchoPlayerPawn;
class UReEchoEncounterHudWidget;
class UReEchoInventoryShopWidget;
class UReEchoLoadoutSelectionWidget;
class UReEchoPlayerHudWidget;
class UReEchoRestartWidget;
class UReEchoSettingsWidget;
class UReEchoStartMenuWidget;
class UReEchoTraitCardChoiceWidget;
class UReEchoStatsWidget;
class UReEchoWeatherWidget;
class UReEchoEchoManagementWidget;
class UReEchoStoredEchoEntryWidget;
class UReEchoEnemyRosterComponent;
class UReEchoAudioService;
class UMaterialInterface;
class UTexture2D;
enum class EReEchoInventoryShopMode : uint8;
struct FReEchoEncounterRuntimeState;
/** 游戏总流程协调器：创建战斗场景，衔接遭遇、构筑选择和结算界面。 */
UCLASS()

class REECHO_API AReEchoGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AReEchoGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	void TogglePauseMenu();
	void ToggleInventoryMenu();
	void ToggleShopMenu();
	void ToggleStatsMenu();

	/** Development-only console commands. Open the console with ~ and run GMHelp. */
	UFUNCTION(Exec)
	void GMHelp();
	UFUNCTION(Exec)
	void GMStatus();
	UFUNCTION(Exec)
	void GMHeal(float Amount = 0.0f);
	UFUNCTION(Exec)
	void GMAddShards(int32 Amount = 100);
	UFUNCTION(Exec)
	void GMWeather(const FString& Scene = TEXT("Clear"));
	UFUNCTION(Exec)
	void GMKillAll();
	UFUNCTION(Exec)
	void GMGotoBoss();

private:
	/** Lets the next-frame World Timer run while retaining menu input and ability blocking. */
	void ResumeWorldForMenuTransition();
	bool EnsureGMCommandAvailable() const;
	void PrintGMResult(const FString& Message, bool bSuccess = true) const;
	UPROPERTY()
	TObjectPtr<AReEchoEncounterDirector> Director;
	UPROPERTY()
	TObjectPtr<AReEchoPlayerPawn> Player;
	UPROPERTY()
	TObjectPtr<ACameraActor> FixedCamera;
	UPROPERTY()
	TObjectPtr<AReEchoArenaSceneActor> ArenaScene;
	UPROPERTY()
	TObjectPtr<AReEchoArenaCameraActor> ArenaCameraActor;
	UPROPERTY()
	TArray<TObjectPtr<AReEchoEchoActor>> Echoes;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UReEchoEnemyRosterComponent> EnemyRoster;

	/** 运行时场地背景，构造期硬引用以确保 Shipping Cook 收录。 */
	UPROPERTY()
	TObjectPtr<UTexture2D> ArenaBackgroundTexture;

	/** 无光照场景材质，避免背景受关卡灯光或 Sprite 渲染代理影响。 */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> ArenaBackgroundMaterial;

	UPROPERTY()
	TObjectPtr<UReEchoRestartWidget> RestartWidget;
	UPROPERTY()
	TObjectPtr<UReEchoSettingsWidget> SettingsWidget;
	UPROPERTY()
	TObjectPtr<UReEchoStartMenuWidget> StartMenuWidget;
	UPROPERTY()
	TObjectPtr<UReEchoLoadoutSelectionWidget> LoadoutSelectionWidget;

	UPROPERTY()
	TObjectPtr<UReEchoInventoryShopWidget> InventoryShopWidget;

	UPROPERTY()
	TObjectPtr<UReEchoStatsWidget> StatsWidget;
	UPROPERTY()
	TObjectPtr<UReEchoEncounterHudWidget> EncounterHudWidget;
	UPROPERTY()
	TObjectPtr<UReEchoPlayerHudWidget> PlayerHudWidget;

	UPROPERTY()
	TObjectPtr<UReEchoWeatherWidget> WeatherWidget;

	bool bRestartScreenIsTerminal = false;
	bool bRestartScreenIsDeath = false;
	bool bAwaitingStartChoice = true;
	bool bQuitConfirmationVisible = false;
	bool bContinueRunAfterShop = false;
	bool bPostTraitShopClosing = false;

	UPROPERTY()
	TObjectPtr<UReEchoTraitCardChoiceWidget> TraitCardChoiceWidget;
	bool bEncounterTransitioning = false;
	bool bEncounterClearedByDefeat = false;
	bool bBossPostEchoPhaseTriggered = false;
	float ArenaSceneWorldHeight = 0.0f;
	float ArenaSceneWorldWidth = 0.0f;
	UFUNCTION()
	void HandleFixedStep(float FixedDeltaSeconds);
	UFUNCTION()
	void HandleEncounterEnded();
	UFUNCTION()
	void HandlePlayerSkill(FVector Position, FName SkillId);

	UFUNCTION()
	void HandlePlayerDeath();

	UFUNCTION()
	void HandleRestartRequested();

	UFUNCTION()
	void HandleResumeRequested();

	UFUNCTION()
	void HandleQuitRequested();

	UFUNCTION()
	void HandleNewGameRequested();

	UFUNCTION()
	void HandleContinueGameRequested();

	UFUNCTION()
	void HandleStartSettingsRequested();

	UFUNCTION()
	void HandlePauseSettingsRequested();

	UFUNCTION()
	void HandleSettingsClosed();

	UFUNCTION()
	void HandleLoadoutConfirmed(FName CharacterId, FName WeaponId);

	UFUNCTION()
	void HandleTraitCardSelected(FName CardId);
	void CreateArena();
	void UpdateWeatherScene(int32 EncounterIndex);
	UReEchoAudioService* GetAudioService() const;
	void SetMusicState(FName StateId) const;
	void SetAmbienceState(FName StateId) const;
	void StopAmbienceState() const;
	void PostAudioEvent(FName EventId, const FVector& WorldLocation = FVector::ZeroVector) const;
	void PostUiEvent(FName EventId) const;
	void RestoreEncounterAudioState();

	UFUNCTION()
	void HandleInventoryShopClosed();

	UFUNCTION()
	void HandleShopPurchaseRequested(FName ItemId);

	UFUNCTION()
	void HandleEchoStoreRequested();

	UFUNCTION()
	void HandleEchoSkipRequested();

	UFUNCTION()
	void HandleEchoReplaceRequested(FGuid RecordingId);

	UFUNCTION()
	void HandleEchoSelectionRequested(const TArray<FGuid>& RecordingIds);

	UFUNCTION()
	void HandleEchoSkipAndCloseRequested();

	void ShowInventoryShopMenu(EReEchoInventoryShopMode Mode);
	/** Opens the post-choice shop outside the card button's Slate input dispatch. */
	void ShowPostTraitShop();

	UFUNCTION()
	void HandleStatsClosed();

	void ShowStatsMenu();

	/** 根据当前运行阶段清理旧对象并启动下一场遭遇。 */
	void BeginNextEncounter();
	void ResumeSavedEncounter();
	FReEchoEncounterRuntimeState CaptureEncounterRuntimeState() const;
	void SpawnEnemies(int32 EncounterIndex);
	bool IsBossEncounter() const;
	void TriggerBossPostEchoPhase(const FReEchoBossPhaseDefinition& PhaseDefinition);
	UFUNCTION()
	void HandleBossIntent(const FReEchoBossIntent& Intent);
	void ClearCombatants();
	void RefreshFogRevealSources();
	/** 结束实时战斗输入并显示死亡、暂停或胜利结算菜单。 */
	void ShowRestartScreen(bool bDeathScreen = true, bool bVictoryScreen = false);
	void ShowSettingsScreen(bool bReturnToStartMenu);
	void ShowTraitCardChoice();
	void ShowStartMenu();
	void ShowLoadoutSelection();
	void BeginSelectedRun();
	void SetGameplayPresentationVisible(bool bVisible);
	void RestoreGameInput();
	void SetPlayerMenuAbilityBlocked(bool bBlocked);

	UFUNCTION()
	void HandleAutomaticAttackRequested();

	UFUNCTION()
	void HandleManualAttackRequested();

	void ApplyAttackModeChoice(bool bAutomatic);
	bool bSettingsReturnToStartMenu = false;
};
