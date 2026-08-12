#pragma once

#include "CoreMinimal.h"
#include "ReEchoUIScreenTypes.generated.h"

/** Stable identities for runtime screens. Gameplay code never needs to know asset paths or viewport Z orders. */
UENUM()
enum class EReEchoUIScreen : uint8
{
	Weather,
	EncounterHud,
	PlayerHud,
	StartMenu,
	Loadout,
	Settings,
	Restart,
	TraitChoice,
	InventoryShop,
	Stats
};

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
