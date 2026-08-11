#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoStartMenuWidget.generated.h"

class SWidget;
class UReEchoIndexedButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoNewGameRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoContinueGameRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoGameSettingRequested);

/** Blocking pre-run panel that offers actions based on resumable-save availability. */
UCLASS()

class REECHO_API UReEchoStartMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoNewGameRequested OnNewGameRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoContinueGameRequested OnContinueGameRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoGameSettingRequested OnGameSettingRequested;

	void InitializeMenu(bool bInHasSavedRun);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshMenu();

	UFUNCTION()
	void HandleMenuAction(int32 ActionIndex);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UReEchoIndexedButton> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<UReEchoIndexedButton> NewGameButton;

	UPROPERTY(Transient)
	TObjectPtr<UReEchoIndexedButton> GameSettingsButton;

	bool bHasSavedRun = false;
};
