#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoRestartWidget.generated.h"

class SWidget;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoRestartRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoResumeRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoQuitRequested);

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

	void SetDeathScreen(bool bInDeathScreen);
	/** 切换到胜利结算模式并显示本轮资源与构筑数量。 */
	void SetVictoryScreen(int32 TimeShards, int32 TraitCount);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshMenuMode();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	bool bDeathScreen = false;
	bool bVictoryScreen = false;
	int32 VictoryTimeShards = 0;
	int32 VictoryTraitCount = 0;
};