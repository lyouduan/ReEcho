#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoLoadoutSelectionWidget.generated.h"

class SWidget;
class UButton;
class UHorizontalBox;
class UImage;
class UReEchoIndexedButton;
class UReEchoLoadoutEntryWidget;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReEchoLoadoutConfirmed, FName, CharacterId, FName, WeaponId);

/** Blocking first-encounter selection for a data-backed character and initial weapon. */
UCLASS()
class REECHO_API UReEchoLoadoutSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoLoadoutSelectionWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoLoadoutConfirmed OnLoadoutConfirmed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

private:
	enum class ESelectionStage : uint8
	{
		Character,
		Weapon
	};

	void BuildWidgetTree();
	void BuildOptionEntries();
	void LoadOptions();
	void RefreshSelection();
	void RefreshSelectionArrow();
	void ApplySelectionArrowVisibility(bool bCharacterStage, int32 PreviewIndex);
	UImage* GetSelectionArrow(bool bCharacterStage, int32 PreviewIndex) const;
	void SetSelectionStage(ESelectionStage NewStage);
	void SelectCharacter(FName CharacterId);
	void ChooseWeapon(FName WeaponId);

	UFUNCTION()
	void HandleCharacterClicked(int32 OptionIndex);

	UFUNCTION()
	void HandleWeaponClicked(int32 OptionIndex);

	UFUNCTION()
	void HandleCharacterPreviewed(int32 OptionIndex);

	UFUNCTION()
	void HandleWeaponPreviewed(int32 OptionIndex);

	UFUNCTION()
	void HandleCharacterHovered(int32 OptionIndex);

	UFUNCTION()
	void HandleWeaponHovered(int32 OptionIndex);

	UFUNCTION()
	void HandleConfirmClicked();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DescriptionTextScale;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmButtonLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CharacterStagePanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> WeaponStagePanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> StageSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DescriptionPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SelectionArrow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterSelectionArrow0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterSelectionArrow1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterSelectionArrow2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponSelectionArrow0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponSelectionArrow1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponSelectionArrow2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponSelectionArrow3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> CharacterRow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> WeaponRow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CharacterButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> WeaponButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoLoadoutEntryWidget>> CharacterEntries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoLoadoutEntryWidget>> WeaponEntries;

	UPROPERTY()
	TSubclassOf<UReEchoLoadoutEntryWidget> EntryWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmButton;

#if WITH_EDITORONLY_DATA
	/** Entry shown as selected in Designer; -1 previews the initial all-bright state. */
	UPROPERTY(EditAnywhere, Category = "Loadout | Designer Preview", meta = (ClampMin = "-1", ClampMax = "3"))
	int32 DesignerPreviewIndex = 3;
#endif

	FName SelectedCharacterId;
	FName SelectedWeaponId;
	ESelectionStage SelectionStage = ESelectionStage::Character;
	bool bFinalConfirmationBroadcast = false;
	bool bShowAnchoredDescription = false;
	TArray<FName> CharacterOptionIds;
	TArray<FName> WeaponOptionIds;
	TMap<FName, FString> CharacterLabels;
	TMap<FName, FString> WeaponLabels;
	TMap<FName, FString> CharacterDescriptions;
	TMap<FName, FString> WeaponDescriptions;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FReEchoLoadoutSelectionFlowTest;
	friend class FReEchoLoadoutSelectionAssetContractTest;
#endif
};
