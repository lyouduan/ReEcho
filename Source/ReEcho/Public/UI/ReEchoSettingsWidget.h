#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoSettingsWidget.generated.h"

class SWidget;
class UButton;
class UReEchoIndexedButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoSettingsClosed);

/** 主菜单与暂停菜单共用的游戏设置页面骨架。 */
UCLASS()

class REECHO_API UReEchoSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoSettingsClosed OnClosed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshCategory();

	UFUNCTION()
	void HandleCategoryClicked(int32 CategoryIndex);

	UFUNCTION()
	void HandleRestoreDefaultsClicked();

	UFUNCTION()
	void HandleApplyAndReturnClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CategoryTitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> GraphicsSettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> AudioSettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> ControlsSettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestoreDefaultsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ApplyAndReturnButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CategoryButtons;

	int32 SelectedCategory = 0;
};
