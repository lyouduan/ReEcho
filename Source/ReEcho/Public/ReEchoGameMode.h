#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ReEchoGameMode.generated.h"
class ACameraActor;
class AReEchoEncounterDirector;
class AReEchoEchoActor;
class AReEchoPlayerPawn;
class UReEchoEncounterHudWidget;
class UReEchoInventoryShopWidget;
class UReEchoPlayerHudWidget;
class UReEchoRestartWidget;
class UReEchoTraitCardChoiceWidget;
class UReEchoStatsWidget;
class UReEchoWeatherWidget;
class UMaterialInterface;
class UTexture2D;
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

private:
	bool EnsureGMCommandAvailable() const;
	void PrintGMResult(const FString& Message, bool bSuccess = true) const;
	UPROPERTY()
	TObjectPtr<AReEchoEncounterDirector> Director;
	UPROPERTY()
	TObjectPtr<AReEchoPlayerPawn> Player;
	UPROPERTY()
	TObjectPtr<ACameraActor> FixedCamera;
	UPROPERTY()
	TArray<TObjectPtr<AReEchoEchoActor>> Echoes;

	/** 运行时场地背景，构造期硬引用以确保 Shipping Cook 收录。 */
	UPROPERTY()
	TObjectPtr<UTexture2D> ArenaBackgroundTexture;

	/** 无光照场景材质，避免背景受关卡灯光或 Sprite 渲染代理影响。 */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> ArenaBackgroundMaterial;

	UPROPERTY()
	TObjectPtr<UReEchoRestartWidget> RestartWidget;

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

	UPROPERTY()
	TObjectPtr<UReEchoTraitCardChoiceWidget> TraitCardChoiceWidget;
	bool bEncounterTransitioning = false;
	bool bEncounterClearedByDefeat = false;
	float ArenaSceneWorldHeight = 0.0f;
	float ArenaSceneWorldWidth = 0.0f;
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
	void UpdateWeatherScene(int32 EncounterIndex);

	UFUNCTION()
	void HandleInventoryShopClosed();

	UFUNCTION()
	void HandleShopPurchaseRequested(FName ItemId);

	void ShowInventoryShopMenu(bool bShowShop);

	UFUNCTION()
	void HandleStatsClosed();

	void ShowStatsMenu();

	/** 根据当前运行阶段清理旧对象并启动下一场遭遇。 */
	void BeginNextEncounter();
	void SpawnEnemies(int32 EncounterIndex);
	void ClearCombatants();
	/** 结束实时战斗输入并显示死亡、暂停或胜利结算菜单。 */
	void ShowRestartScreen(bool bDeathScreen = true, bool bVictoryScreen = false);
	void ShowTraitCardChoice();
	void RestoreGameInput();
	void SetPlayerMenuAbilityBlocked(bool bBlocked);
};
