#pragma once

#include "Data/ReEchoCsvDataRegistry.h"
#include "Data/ReEchoDataAssets.h"
#include "UObject/UObjectGlobals.h"

namespace ReEchoDataAssetTestUtils
{
inline const UReEchoGameDataCatalog* LoadDefaultCatalog()
{
	return LoadObject<UReEchoGameDataCatalog>(
	    nullptr, TEXT("/Game/ReEcho/DataAsset/Gameplay/DA_ReEchoGameDataCatalog.DA_ReEchoGameDataCatalog"));
}

inline UReEchoGameDataCatalog* DuplicateCatalog()
{
	const UReEchoGameDataCatalog* Source = LoadDefaultCatalog();
	if (!Source)
	{
		return nullptr;
	}
	UReEchoGameDataCatalog* Copy = DuplicateObject<UReEchoGameDataCatalog>(Source, GetTransientPackage());
	Copy->Core = DuplicateObject<UReEchoCoreDataAsset>(Source->Core, Copy);
	Copy->Cards = DuplicateObject<UReEchoCardDataAsset>(Source->Cards, Copy);
	Copy->Elements = DuplicateObject<UReEchoElementDataAsset>(Source->Elements, Copy);
	Copy->Weapons = DuplicateObject<UReEchoWeaponDataAsset>(Source->Weapons, Copy);
	Copy->Enemies = DuplicateObject<UReEchoEnemyDataAsset>(Source->Enemies, Copy);
	Copy->Encounters = DuplicateObject<UReEchoEncounterDataAsset>(Source->Encounters, Copy);
	Copy->Shop = DuplicateObject<UReEchoShopDataAsset>(Source->Shop, Copy);
	return Copy;
}
}
