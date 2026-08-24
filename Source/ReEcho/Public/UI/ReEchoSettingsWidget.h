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
class UComboBoxString;
class UImage;
class UTexture2D;
class UReEchoAudioService;
class UWidget;
enum class EReEchoAudioBus : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoSettingsClosed);

/** 主菜单与暂停菜单共用的游戏设置页面骨架。 */
UCLASS()

class REECHO_API UReEchoSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoSettingsWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoSettingsClosed OnClosed;

	/** Non-music buses represented by the delivered compact effects slider. */
	static TConstArrayView<EReEchoAudioBus> GetCompactEffectsBuses();

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
	void BuildInteractiveSettingsControls();
	void RefreshGraphicsControls();
	bool UsesCompactAudioLayout() const;
	void UpdateVolumeVisual(UImage* FillImage, UTextBlock* PercentText, float Value) const;
	void PostUiEvent(FName EventId) const;
	UComboBoxString* AddComboBoxOverlay(FName ComboName, FName FieldName, FName ValueName, FName ArrowName);
	USlider* AddSliderOverlay(FName SliderName, FName TrackName);
	UReEchoAudioService* GetAudioService() const;

	UFUNCTION()
	void HandleCategoryClicked(int32 CategoryIndex);

	UFUNCTION()
	void HandleRestoreDefaultsClicked();

	UFUNCTION()
	void HandleApplyAndReturnClicked();

	UFUNCTION()
	void HandleCloseClicked();

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

	UFUNCTION()
	void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleDisplayModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleVSyncChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleAudioOutputChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleBrightnessChanged(float Value);
	UFUNCTION()
	void HandleSliderInteractionFinished();
	UFUNCTION()
	UWidget* GenerateComboBoxItem(FString Item);

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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsCloseButton;

	/** Paper-note art selected by runtime state; layout remains authored in WBP. */
	UPROPERTY()
	TObjectPtr<UTexture2D> SettingsTabLightTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> SettingsTabDarkTexture;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CategoryButtons;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> AudioPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GraphicsPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ControlsPanel;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DetailContent;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> MasterVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> MusicVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> AmbienceVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> CombatSfxVolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USlider> UiSfxVolumeSlider;
	UPROPERTY(Transient) TObjectPtr<USlider> GraphicsBrightnessSlider;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> MasterVolumeFill;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> MusicVolumeFill;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> CombatVolumeFill;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> GraphicsBrightnessFill;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MasterVolumePercent;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MusicVolumePercent;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CombatVolumePercent;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GraphicsValue4;

	UPROPERTY(Transient) TObjectPtr<UComboBoxString> GraphicsResolutionComboBox;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> GraphicsDisplayModeComboBox;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> GraphicsQualityComboBox;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> GraphicsVSyncComboBox;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> AudioOutputComboBox;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> MasterMuteCheckBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> MusicMuteCheckBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> AmbienceMuteCheckBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> CombatSfxMuteCheckBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> UiSfxMuteCheckBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> DiagnosticToneCheckBox;

	int32 SelectedCategory = 0;
	bool bAudioSettingsApplied = false;
	bool bGraphicsSettingsApplied = false;
	bool bRefreshingAudioControls = false;
	bool bRefreshingGraphicsControls = false;
	float InitialDisplayGamma = 2.2f;
	float PendingBrightness = 0.65f;
};
