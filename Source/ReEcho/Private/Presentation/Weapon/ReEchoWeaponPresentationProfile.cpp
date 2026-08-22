#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"

bool UReEchoWeaponPresentationProfile::IsSlotValid(const FReEchoWeaponVfxSlot& Slot) const
{
	return !Slot.bEnabled || !Slot.System.IsNull();
}
