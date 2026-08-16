#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoSettingsWidget.generated.h"

class SWidget;
class UButton;
class UReEchoIndexedButton;
class UTextBlock;
class UVerticalBox;
class USlider;
class UCheckBox;
class UReEchoAudioService;

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
	virtual void NativeDestruct() override;

private:
	void BuildWidgetTree();
	void RefreshCategory();

	// Plan34: typed optional WBP bindings with a C++ fallback panel.
	void BuildAudioPanel();
	void BindAudioControls();
	void RefreshAudioControls();
	UReEchoAudioService* GetAudioService() const;

	UFUNCTION()
	void HandleCategoryClicked(int32 CategoryIndex);

	UFUNCTION()
	void HandleRestoreDefaultsClicked();

	UFUNCTION()
	void HandleApplyAndReturnClicked();

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);
	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);
	UFUNCTION()
	void HandleAmbienceVolumeChanged(float Value);
	UFUNCTION()
	void HandleCombatSfxVolumeChanged(float Value);
	UFUNCTION()
	void HandleUiSfxVolumeChanged(float Value);
	UFUNCTION()
	void HandleMasterMuteChanged(bool bChecked);
	UFUNCTION()
	void HandleMusicMuteChanged(bool bChecked);
	UFUNCTION()
	void HandleAmbienceMuteChanged(bool bChecked);
	UFUNCTION()
	void HandleCombatSfxMuteChanged(bool bChecked);
	UFUNCTION()
	void HandleUiSfxMuteChanged(bool bChecked);
	UFUNCTION()
	void HandleDiagnosticToneChanged(bool bChecked);

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> AudioPanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DetailContent;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> MasterVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> MusicVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> AmbienceVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> CombatSfxVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> UiSfxVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> MasterMuteCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> MusicMuteCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> AmbienceMuteCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> CombatSfxMuteCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> UiSfxMuteCheck;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> DiagnosticToneCheck;

	int32 SelectedCategory = 0;
	bool bAudioSettingsApplied = false;
	bool bRefreshingAudioControls = false;
};
