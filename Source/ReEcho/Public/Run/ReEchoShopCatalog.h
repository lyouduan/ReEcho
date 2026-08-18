#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"

enum class EReEchoShopOfferType : uint8
{
	RunItem,
	WeaponPart
};

struct REECHO_API FReEchoShopOffer
{
	FName ItemId;
	FText DisplayName;
	FText EffectText;
	int32 Price = 0;
	EReEchoShopOfferType Type = EReEchoShopOfferType::RunItem;
	FName ContentId;
	FName SlotTypeId;
};

struct REECHO_API FReEchoWeaponSlotShopView
{
	FName SlotTypeId;
	FText DisplayName;
	int32 Capacity = 0;
	bool bRequired = false;
};

struct REECHO_API FReEchoWeaponPartShopView
{
	FName WeaponId;
	FText WeaponDisplayName;
	TArray<FReEchoShopOffer> Offers;
	TArray<FReEchoShopOffer> OwnedParts;
	TArray<FReEchoWeaponSlotShopView> Slots;
	TArray<FReEchoEquippedPartSnapshot> EquippedParts;
};

inline const TArray<FReEchoShopOffer>& GetReEchoShopCatalog()
{
	static const TArray<FReEchoShopOffer> Offers = {
	    {TEXT("SHOP_RUSTED_SCISSORS"),
	     NSLOCTEXT("ReEcho", "ShopRustedScissors", "生锈剪刀"),
	     NSLOCTEXT("ReEcho", "ShopRustedScissorsEffect", "物理攻击 +2"),
	     15},
	    {TEXT("SHOP_DREAM_FRUIT"),
	     NSLOCTEXT("ReEcho", "ShopDreamFruit", "噩梦果实"),
	     NSLOCTEXT("ReEcho", "ShopDreamFruitEffect", "最大生命 +10"),
	     20},
	    {TEXT("SHOP_BLACK_FEATHER"),
	     NSLOCTEXT("ReEcho", "ShopBlackFeather", "黑羽毛"),
	     NSLOCTEXT("ReEcho", "ShopBlackFeatherEffect", "移动速度 +10%"),
	     20},
	    {TEXT("SHOP_OLD_COIN"),
	     NSLOCTEXT("ReEcho", "ShopOldCoin", "古老硬币"),
	     NSLOCTEXT("ReEcho", "ShopOldCoinEffect", "回响效率 +10%"),
	     25},
	    {TEXT("SHOP_REPLAY_UNLOCK"),
	     NSLOCTEXT("ReEcho", "ShopReplayUnlock", "指定回放解锁"),
	     NSLOCTEXT("ReEcho", "ShopReplayUnlockEffect", "在收藏的回响中选择最多 3 场自动回放"),
	     30}};
	return Offers;
}
