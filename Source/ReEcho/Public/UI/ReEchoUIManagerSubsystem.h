#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/Framework/ReEchoUIScreenTypes.h"
#include "ReEchoUIManagerSubsystem.generated.h"

class UUserWidget;
class APlayerController;

/** Owns viewport-layer policy and the lifecycle of registered runtime widgets. */
UCLASS()

class REECHO_API UReEchoUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UReEchoUIManagerSubsystem();

	UUserWidget* CreateScreen(APlayerController* PlayerController, EReEchoUIScreen Screen);
	UUserWidget* GetScreen(EReEchoUIScreen Screen) const;
	bool IsScreenOpen(EReEchoUIScreen Screen) const;
	void CloseScreen(EReEchoUIScreen Screen);
	void AddToLayer(UUserWidget* Widget, EReEchoUILayer Layer);
	void ConfigureMenuInput(APlayerController* PlayerController, UUserWidget* Widget, bool bUIOnly) const;
	void ConfigureGameplayInput(APlayerController* PlayerController) const;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FReEchoUIManagerSubsystemResetOnTravelTest;
	friend class FReEchoPauseOverlayLayerPolicyTest;
#endif

	void ResetScreens();
	void HandlePreLoadMap(const FString& MapName);

	FDelegateHandle PreLoadMapHandle;

	static int32 GetLayerZOrder(EReEchoUILayer Layer);
	static EReEchoUILayer GetScreenLayer(EReEchoUIScreen Screen);
	TSubclassOf<UUserWidget> GetScreenClass(EReEchoUIScreen Screen) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> ManagedWidgets;

	UPROPERTY(Transient)
	TMap<EReEchoUIScreen, TObjectPtr<UUserWidget>> ActiveScreens;

	UPROPERTY()
	TMap<EReEchoUIScreen, TSubclassOf<UUserWidget>> ScreenClasses;
};
