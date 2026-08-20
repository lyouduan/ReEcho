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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoStartAboutRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoStartQuitRequested);

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

	UPROPERTY(BlueprintAssignable)
	FReEchoStartAboutRequested OnAboutRequested;

	UPROPERTY(BlueprintAssignable)
	FReEchoStartQuitRequested OnQuitRequested;

	void InitializeMenu(bool bInHasSavedRun);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void EnsureOpaqueBackground();
	void RefreshMenu();

	UFUNCTION()
	void HandleMenuAction(int32 ActionIndex);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> ContinueButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> NewGameButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> GameSettingsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> AboutButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoIndexedButton> QuitButton;

	bool bHasSavedRun = false;
};
