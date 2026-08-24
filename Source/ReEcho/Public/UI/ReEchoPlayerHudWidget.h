#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoPlayerHudWidget.generated.h"

class SWidget;
class UImage;
class UProgressBar;
class UReEchoCombatantComponent;
class UReEchoCombatEventsComponent;
class UReEchoPlayerScreenFeedbackWidget;
struct FReEchoDamageEvent;
class UTextBlock;
class UTexture2D;

/** 左上角玩家状态 HUD：头像、当前生命值与最大生命值。 */
UCLASS()

class REECHO_API UReEchoPlayerHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UReEchoPlayerHudWidget(const FObjectInitializer& ObjectInitializer);

	void InitializePlayerHud(UReEchoCombatantComponent* InCombatant,
	                         UReEchoCombatEventsComponent* InCombatEvents,
	                         UTexture2D* InPortraitTexture);

	/** 更新本局实时持有的时间碎片数量。 */
	void SetTimeShards(int32 InTimeShards);

#if WITH_DEV_AUTOMATION_TESTS
	bool IsBoundToCombatEventsForTests(const UReEchoCombatEventsComponent* InCombatEvents) const;

	UClass* GetScreenFeedbackClassForTests() const;

	int32 GetTimeShardsForTests() const;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildWidgetTree();
	void Refresh();
	void BindCombatant(UReEchoCombatantComponent* InCombatant);
	void BindCombatEvents(UReEchoCombatEventsComponent* InCombatEvents);
	void EnsureScreenFeedbackWidget();

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaximumHealth);

	UFUNCTION()
	void HandlePlayerHurt(const FReEchoDamageEvent& Event);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PlayerPortrait;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PlayerHealthProgress;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PlayerHealthFill;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlayerHealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeShardText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UReEchoPlayerScreenFeedbackWidget> PlayerScreenFeedback;

	UPROPERTY(EditDefaultsOnly, Category = "Player HUD|Screen Feedback")
	TSubclassOf<UReEchoPlayerScreenFeedbackWidget> PlayerScreenFeedbackClass;

	TWeakObjectPtr<UReEchoCombatantComponent> Combatant;
	TWeakObjectPtr<UReEchoCombatEventsComponent> CombatEvents;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PortraitTexture;

	int32 CurrentTimeShards = 0;
};
