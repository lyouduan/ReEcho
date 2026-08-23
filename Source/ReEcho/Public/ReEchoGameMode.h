#pragma once
#include "CoreMinimal.h"
#include "Combat/ReEchoCombatTypes.h"
#include "Encounter/ReEchoEncounterRuntime.h"
#include "Enemies/ReEchoEnemyTypes.h"
#include "GameFramework/GameModeBase.h"
#include "UI/ReEchoAboutWidget.h"
#include "ReEchoGameMode.generated.h"
class ACameraActor;
class AReEchoArenaCameraActor;
class AReEchoArenaSceneActor;
class AReEchoEncounterDirector;
class AReEchoEchoActor;
class AReEchoEnemyActor;
class AReEchoPlayerPawn;
class UReEchoEncounterHudWidget;
class UReEchoInventoryShopWidget;
class UReEchoRunSubsystem;
enum class EReEchoInventoryShopMode : uint8;
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
class UReEcho2DPresentationCatalog;
class UReEchoEnemyGameplayClassRegistry;
class UReEchoAudioService;
class UMaterialInterface;
class UTexture2D;
enum class EReEchoInventoryShopMode : uint8;
struct FReEchoEncounterRuntimeState;
struct FReEchoMinimapView;
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
	/** Toggles final-damage immunity on the current player. */
	UFUNCTION(Exec)
	void GMGod(const FString& Mode = TEXT("Toggle"));
	UFUNCTION(Exec)
	void GMAddShards(int32 Amount = 100);
	UFUNCTION(Exec)
	void GMSetShards(int32 Amount = 0);
	UFUNCTION(Exec)
	void GMWeather(const FString& Scene = TEXT("Clear"));
	UFUNCTION(Exec)
	void GMEndEncounter();
	UFUNCTION(Exec)
	void GMKillAll();
	UFUNCTION(Exec)
	void GMSpawnFox(float Distance = 350.0f);
	UFUNCTION(Exec)
	void GMGotoBoss();
	UFUNCTION(Exec)
	void GMGrantCard(FName CardId);
	/** Locks every subsequent player hit to one element. Use None to restore weapon-authored elements. */
	UFUNCTION(Exec)
	void GMElement(const FString& Element = TEXT("Flame"));
	/** Prepares and triggers one authored reaction through the production resolver on the nearest living enemy. */
	UFUNCTION(Exec)
	void GMReaction(const FString& Reaction = TEXT("Burn"), float Damage = 10.0f);

	/** Single Encounter-owned gate for ranged burst windows and elite special concurrency. */
	bool CanStartEnemySpecial(FName EnemyId, int32 SpawnIndex, float WorldTimeSeconds);
	void NotifyEnemySpecialStarted(FName EnemyId, int32 SpawnIndex, float WorldTimeSeconds);

private:
	/** Lets the next-frame World Timer run while retaining menu input and ability blocking. */
	void ResumeWorldForMenuTransition();
	bool EnsureGMCommandAvailable() const;
	void PrintGMResult(const FString& Message, bool bSuccess = true) const;
	AReEchoEnemyActor* FindNearestLivingEnemyForGM() const;
	FReEchoAttackIdentity MakeGMElementAttack();
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
	UPROPERTY()
	TObjectPtr<UReEcho2DPresentationCatalog> PresentationCatalog;
	UPROPERTY()
	TObjectPtr<UReEchoEnemyGameplayClassRegistry> EnemyGameplayClassRegistry;
	int64 GMElementAttackSequence = 0;

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
	TObjectPtr<UReEchoAboutWidget> AboutWidget;
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
	bool bExitToMainMenuAfterConfirmation = false;
	bool bContinueRunAfterShop = false;
	bool bPostTraitShopClosing = false;

	UPROPERTY()
	TObjectPtr<UReEchoTraitCardChoiceWidget> TraitCardChoiceWidget;
	bool bEncounterTransitioning = false;
	/** #9 倒计时归零到弹出选卡之间的短暂停顿定时器（让"0"可见）。 */
	FTimerHandle EncounterEndSettleTimerHandle;
	bool bEncounterClearedByDefeat = false;
	bool bBossPostEchoPhaseTriggered = false;
	float ArenaSceneWorldHeight = 0.0f;
	float ArenaSceneWorldWidth = 0.0f;
	FReEchoEncounterWaveScheduler EncounterWaveScheduler;
	FName CurrentEncounterId = NAME_None;
	int32 EncounterSpawnSequence = 0;
	int32 EnemySpawnIndex = 0;
	TArray<FVector> EncounterSpawnLocations;
	TArray<float> RecentRangedBurstWorldTimes;
	TArray<FReEchoPendingSpawnBatchState> PendingSpawnBatches;
	UFUNCTION()
	void HandleFixedStep(float FixedDeltaSeconds);
	UFUNCTION()
	void HandleEncounterEnded();
	/** #9 倒计时显示到 0 后，再收起 HUD 并弹出选卡/结算界面的延时回调。 */
	UFUNCTION()
	void ProceedToPostEncounterUI();
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
	void HandleExitToMainMenuRequested();

	UFUNCTION()
	void HandleExitWithoutSavingRequested();

	UFUNCTION()
	void HandleCancelExitRequested();

	UFUNCTION()
	void HandleNewGameRequested();

	UFUNCTION()
	void HandleContinueGameRequested();

	UFUNCTION()
	void HandleStartSettingsRequested();

	UFUNCTION()
	void HandleStartAboutRequested();

	UFUNCTION()
	void HandleStartQuitRequested();

	UFUNCTION()
	void HandlePauseSettingsRequested();

	UFUNCTION()
	void HandleSettingsClosed();

	UFUNCTION()
	void HandleAboutClosed();

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
	void HandleShopRefreshRequested();
	void RefreshShopPresentation(UReEchoRunSubsystem* RunSubsystem, EReEchoInventoryShopMode Mode);

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
	TSubclassOf<AReEchoEnemyActor> ResolveEnemyClass(FName PresentationId) const;
	FReEchoEncounterRuntimeState CaptureEncounterRuntimeState() const;
	bool ConfigureEncounterSpawns(int32 EncounterIndex);
	void ProcessScheduledSpawnEvents(float EncounterSeconds);
	void PrepareScheduledSpawnBatch(const FReEchoScheduledSpawnEvent& Event);
	void SpawnScheduledBatch(const FReEchoScheduledSpawnEvent& Event);
	bool SpawnConfiguredEnemy(FName EnemyId, const FVector& SpawnLocation, int32 CombatIndex = INDEX_NONE);
	int32 GetTotalEncounterCount() const;
	bool IsBossEncounter() const;
	void TriggerBossPostEchoPhase(const FReEchoBossPhaseDefinition& PhaseDefinition);
	UFUNCTION()
	void HandleBossIntent(const FReEchoBossIntent& Intent);
	bool ResolveNextStageTransition(FReEchoStageTransitionDecision& OutDecision, FString& OutError) const;
	void PrepareEncounterIntermission();
	void SetEnemyEncounterSimulationSuspended(bool bSuspended);
	FVector ResolveStageEntryLocation() const;
	void ClearEnemyRoster();
	void ClearEchoes();
	void ClearCombatants();
	void RefreshFogRevealSources();
	/** 结束实时战斗输入并显示死亡、暂停或胜利结算菜单。 */
	void ShowRestartScreen(bool bDeathScreen = true, bool bVictoryScreen = false);
	void ShowSettingsScreen(bool bReturnToStartMenu);
	void ShowAboutScreen(bool bReturnToStartMenu);
	void ShowTraitCardChoice();
	void ShowStartMenu();
	void ShowLoadoutSelection();
	void RequestBeginSelectedRun();
	void HandleRuntimeAssetPreloadComplete();
	void BeginSelectedRun();
	void SetGameplayPresentationVisible(bool bVisible);
	void RestoreGameInput();
	void SetPlayerMenuAbilityBlocked(bool bBlocked);

	UFUNCTION()
	void HandleAutomaticAttackRequested();

	/** 每帧从 Player/Echoes 收集数据填充小地图视图（Plan 64）。 */
	void BuildMinimapView(FReEchoMinimapView& OutView) const;

	UFUNCTION()
	void HandleManualAttackRequested();

	void ApplyAttackModeChoice(bool bAutomatic);
	void CompletePauseExit();
	bool bSettingsReturnToStartMenu = false;
	bool bAboutReturnToStartMenu = false;
	bool bBeginSelectedRunRequested = false;
	bool bBeginSelectedRunStarted = false;
};
