#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ReEchoUIManagerSubsystem.generated.h"

class UUserWidget;
class APlayerController;

/** Stable viewport layers for all ReEcho runtime UI. */
UENUM()
enum class EReEchoUILayer : uint8
{
	Weather,
	GameplayHud,
	PlayerHud,
	BuildChoice,
	Screen,
	Pause,
	Start,
	Loadout,
	Settings
};

/** Owns viewport-layer policy and the lifecycle of registered runtime widgets. */
UCLASS()

class REECHO_API UReEchoUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void AddToLayer(UUserWidget* Widget, EReEchoUILayer Layer);
	void ConfigureMenuInput(APlayerController* PlayerController, UUserWidget* Widget, bool bUIOnly) const;
	void ConfigureGameplayInput(APlayerController* PlayerController) const;
	virtual void Deinitialize() override;

private:
	static int32 GetLayerZOrder(EReEchoUILayer Layer);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> ManagedWidgets;
};
