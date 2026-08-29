#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Run/ReEchoRunStatsTracker.h"
#include "Run/ReEchoShopCatalog.h"
#include "UI/ReEchoAttackModeWidget.h"
#include "ReEchoRestartWidget.generated.h"

class SWidget;
class UButton;
class UCanvasPanel;
class UImage;
class UReEchoButtonVisualFeedback;
class UTextBlock;
class UVerticalBox;

UENUM()
enum class EReEchoRestartScreenMode : uint8
{
	Pause,
	Death,
	Victory
};

UENUM()
enum class EReEchoQuitPromptState : uint8
{
	None,
	Confirm,
	SaveFailed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoRestartRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoResumeRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoQuitRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoSettingsRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoExitToMainMenuRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoExitWithoutSavingRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoCancelExitRequested);

/** 运行时结算菜单：复用同一界面呈现暂停、死亡和胜利状态。 */
UCLASS()

class REECHO_API UReEchoRestartWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoRestartRequested OnRestartRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoResumeRequested OnResumeRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoQuitRequested OnQuitRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoSettingsRequested OnSettingsRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoExitToMainMenuRequested OnExitToMainMenuRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoExitWithoutSavingRequested OnExitWithoutSavingRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoCancelExitRequested OnCancelExitRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoAttackModeRequested OnAutomaticAttackRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoAttackModeRequested OnManualAttackRequested;

	/** 切换到失败结算模式；统计项均来自当前运行，不展示伪造的击杀或金币数据。 */
	void SetDeathScreen(bool bInDeathScreen,
	                    int32 EncounterIndex = 0,
	                    int32 TimeShards = 0,
	                    int32 TraitCount = 0,
	                    FName InCharacterId = NAME_None);
	/** Death result overload that projects the highest-tier owned card icons into the five authored slots. */
	void SetDeathScreen(bool bInDeathScreen,
	                    int32 EncounterIndex,
	                    int32 TimeShards,
	                    int32 TraitCount,
	                    FName InCharacterId,
	                    const TArray<FReEchoShopOffer>& OwnedCards);
	/** 切换到胜利结算模式并显示本轮资源与构筑数量。 */
	void SetVictoryScreen(int32 TimeShards, int32 TraitCount, FName InCharacterId = NAME_None);
	/** Victory overload mirroring the defeat surface: projects owned card icons and run statistics. */
	void SetVictoryScreen(int32 TimeShards, int32 TraitCount, FName InCharacterId, const TArray<FReEchoShopOffer>& OwnedCards);
	/** Pause-menu second step: only return to the game or confirm exit remain actionable. */
	void SetQuitConfirmation(bool bInQuitConfirmation, bool bInExitToMainMenu = false, int32 InEncounterIndex = 0);
	void ShowSaveFailure();
	void SetAutomaticAttackMode(bool bAutomatic);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshMenuMode();
	void RefreshSettlementCharacterImages();
	void RefreshDefeatCardSlots();
	void RefreshCardIconSlots(const TArray<FString>& TexturePaths, const TCHAR* SlotNamePrefix);
	void CaptureRunStats();
	void RefreshRunStatsValues();
	void EnsureAttackModeWidget();
	void BindFormalResultButtonFeedback();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleDefeatMainMenuClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleAutomaticAttackClicked();

	UFUNCTION()
	void HandleManualAttackClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> MenuContent;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PauseSettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResumeButtonLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RestartButtonLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuitButtonText;

	/** Designer-authored formal victory surface. Runtime only projects state into these optional bindings. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> VictoryCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VictoryEncounterValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VictoryTimeShardsValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VictoryTraitCountValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> VictoryContinueButton;

	/** Existing authored character image; runtime replaces only its Brush, never its WBP geometry. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ArtVictoryCharacterFormal;

	/** Designer-authored formal defeat surface. Runtime only projects state into these optional bindings. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> DefeatCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefeatEncounterValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefeatTimeShardsValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefeatTraitCountValue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DefeatRestartButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DefeatMainMenuButton;

	/** Existing authored character image; runtime replaces only its Brush, never its WBP geometry. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ArtDefeatCharacterFormal;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtRestartDialogPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtVictoryContinueButtonFormal;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtDefeatRestartButtonFormal;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtDefeatMainMenuButtonFormal;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VictoryContinueLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DefeatRestartLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DefeatMainMenuLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtPauseDimmer;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtPausePrimaryButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtPauseSecondaryButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtPauseTertiaryButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtPauseSettings;

	/** Run statistics authored on both settlement surfaces; runtime only projects text into these bindings. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VictoryEchoDamageValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VictoryPlayerDamageValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VictoryReactionCountValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VictoryMaxHitValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VictoryKillCountValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DefeatEchoDamageValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DefeatPlayerDamageValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DefeatReactionCountValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DefeatMaxHitValue;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DefeatKillCountValue;

	UPROPERTY(Transient)
	TObjectPtr<UReEchoAttackModeWidget> AttackModeWidget;

	/** Keeps hover bindings for the formal result buttons alive without taking layout ownership from the WBP. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoButtonVisualFeedback>> FormalResultButtonFeedback;

	EReEchoRestartScreenMode ScreenMode = EReEchoRestartScreenMode::Pause;
	EReEchoQuitPromptState QuitPromptState = EReEchoQuitPromptState::None;
	int32 VictoryTimeShards = 0;
	int32 VictoryTraitCount = 0;
	int32 DefeatEncounterIndex = 0;
	int32 DefeatTimeShards = 0;
	int32 DefeatTraitCount = 0;
	TArray<FString> DefeatCardIconTexturePaths;
	/** Mirrors the defeat card slots on the victory surface, using the DesignerVictoryCardIcon%d naming. */
	TArray<FString> VictoryCardIconTexturePaths;
	/** Snapshot of the independent run statistics captured when a settlement screen is opened. */
	FReEchoRunCombatStats SettlementStats;
	FName SettlementCharacterId = NAME_None;
	int32 PauseEncounterIndex = 0;
	bool bAutomaticAttackMode = true;
	bool bExitToMainMenu = false;
};
