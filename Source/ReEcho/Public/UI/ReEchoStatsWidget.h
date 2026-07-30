#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/ReEchoTypes.h"
#include "ReEchoStatsWidget.generated.h"

class SWidget;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReEchoStatsClosed);

UCLASS()

class REECHO_API UReEchoStatsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoStatsWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable)
	FReEchoStatsClosed OnClosed;

	void InitializeStats(const FReEchoStatBlock& PlayerStats,
	                     float PlayerCurrentHealth,
	                     const FReEchoStatBlock& EchoStats,
	                     float EchoCurrentHealth,
	                     bool bHasEcho);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void Refresh();
	FText FormatStats(const FReEchoStatBlock& Stats, float CurrentHealth) const;

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY()
	TObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PlayerStatsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EchoStatsText;

	FReEchoStatBlock CurrentPlayerStats;
	FReEchoStatBlock CurrentEchoStats;
	float CurrentPlayerHealth = 0.0f;
	float CurrentEchoHealth = 0.0f;
	bool bCurrentHasEcho = false;
};
