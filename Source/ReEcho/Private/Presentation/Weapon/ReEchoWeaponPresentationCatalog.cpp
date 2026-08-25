#include "Presentation/Weapon/ReEchoWeaponPresentationCatalog.h"

#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"

UReEchoWeaponPresentationProfile* UReEchoWeaponPresentationCatalog::ResolveProfile(const FName WeaponVisualKey) const
{
	const TSoftObjectPtr<UReEchoWeaponPresentationProfile>* Entry = Profiles.Find(WeaponVisualKey);
	return Entry ? Entry->LoadSynchronous() : nullptr;
}

void UReEchoWeaponPresentationCatalog::GatherPreloadAssetPaths(TArray<FString>& OutPaths) const
{
	for (const TPair<FName, TSoftObjectPtr<UReEchoWeaponPresentationProfile>>& Entry : Profiles)
	{
		if (!Entry.Value.IsNull())
		{
			OutPaths.Add(Entry.Value.ToSoftObjectPath().ToString());
		}
		if (const UReEchoWeaponPresentationProfile* Profile = Entry.Value.LoadSynchronous())
		{
			auto AddSoftPath = [&OutPaths](const auto& Asset)
			{
				if (!Asset.IsNull())
				{
					OutPaths.Add(Asset.ToSoftObjectPath().ToString());
				}
			};
			AddSoftPath(Profile->HeldTexture);
			AddSoftPath(Profile->LegacyAttackTexture);
			AddSoftPath(Profile->Charge.System);
			AddSoftPath(Profile->Travel.System);
			AddSoftPath(Profile->DamageApplied.System);
			AddSoftPath(Profile->AttackCommitted.System);
		}
	}
}
