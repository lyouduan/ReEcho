#pragma once

#include "CoreMinimal.h"

struct REECHO_API FReEchoShopOffer
{
	FName ItemId;
	FText DisplayName;
	FText EffectText;
	int32 Price = 0;
};

inline const TArray<FReEchoShopOffer>& GetReEchoShopCatalog()
{
	static const TArray<FReEchoShopOffer> Offers = {{TEXT("SHOP_RUSTED_SCISSORS"),
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
	                                                 25}};
	return Offers;
}
