#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ReEchoAttackModeWidget.h"
#include "ReEchoRestartWidget.generated.h"

class SWidget;
class UButton;
class UImage;
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
	FReEchoAttackModeRequested OnAutomaticAttackRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoAttackModeRequested OnManualAttackRequested;

	void SetDeathScreen(bool bInDeathScreen);
	/** 切换到胜利结算模式并显示本轮资源与构筑数量。 */
	void SetVictoryScreen(int32 TimeShards, int32 TraitCount);
	/** Pause-menu second step: only return to the game or confirm exit remain actionable. */
	void SetQuitConfirmation(bool bInQuitConfirmation);
	void ShowSaveFailure();
	void SetAutomaticAttackMode(bool bAutomatic);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshMenuMode();
	void EnsureAttackModeWidget();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleAutomaticAttackClicked();

	UFUNCTION()
	void HandleManualAttackClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> MenuContent;

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
	TObjectPtr<UTextBlock> QuitButtonText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtRestartDialogPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtRestartCharacter;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtResultSummaryPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtSelectedCardsPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtVictoryTitle;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> ArtDefeatTitle;

	UPROPERTY(Transient)
	TObjectPtr<UReEchoAttackModeWidget> AttackModeWidget;

	EReEchoRestartScreenMode ScreenMode = EReEchoRestartScreenMode::Pause;
	EReEchoQuitPromptState QuitPromptState = EReEchoQuitPromptState::None;
	int32 VictoryTimeShards = 0;
	int32 VictoryTraitCount = 0;
	bool bAutomaticAttackMode = true;
};
