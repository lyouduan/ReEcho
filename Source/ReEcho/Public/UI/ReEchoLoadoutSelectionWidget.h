#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoLoadoutSelectionWidget.generated.h"

class SWidget;
class UButton;
class UReEchoIndexedButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReEchoLoadoutConfirmed, FName, CharacterId, FName, WeaponId);

/** Blocking first-encounter selection for a data-backed character and initial weapon. */
UCLASS()

class REECHO_API UReEchoLoadoutSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FReEchoLoadoutConfirmed OnLoadoutConfirmed;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void LoadOptions();
	void RefreshSelection();
	void SelectCharacter(FName CharacterId);
	void ChooseWeapon(FName WeaponId);

	UFUNCTION()
	void HandleCharacterClicked(int32 OptionIndex);

	UFUNCTION()
	void HandleWeaponClicked(int32 OptionIndex);

	UFUNCTION()
	void HandleConfirmClicked();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> CharacterButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UReEchoIndexedButton>> WeaponButtons;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ConfirmButton;

	FName SelectedCharacterId;
	FName SelectedWeaponId;
	TArray<FName> CharacterOptionIds;
	TArray<FName> WeaponOptionIds;
	TMap<FName, FString> CharacterLabels;
	TMap<FName, FString> WeaponLabels;
};
