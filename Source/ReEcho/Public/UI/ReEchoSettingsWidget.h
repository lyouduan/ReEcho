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

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CategoryTitleText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CategoryButtons;

	int32 SelectedCategory = 0;
};
