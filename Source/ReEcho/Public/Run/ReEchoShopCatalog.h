#pragma once

#include "CoreMinimal.h"
#include "Core/ReEchoTypes.h"

enum class EReEchoShopOfferType : uint8
{
	RunItem,
	WeaponPart,
	BuildCard,
	Weapon
};

enum class EReEchoShopOfferKind : uint8
{
	Part,
	Weapon
};

inline constexpr int32 ReEchoShopRefreshPrice = 10;
inline constexpr int32 ReEchoShopOfferCountPerGroup = 3;

struct REECHO_API FReEchoShopOffer
{
	FName ItemId;
	FText DisplayName;
	FText EffectText;
	int32 Price = 0;
	EReEchoShopOfferType Type = EReEchoShopOfferType::RunItem;
	FName ContentId;
	FName SlotTypeId;
	int32 Tier = 0;
	FString IconTexturePath;   // 武器 Offer 填对应配图资产路径(ResolveHeldTexturePath)，为空则回退默认卡片图标
};

// One fixed weapon/part shop slot offer (left = universal rune, mid/right = weighted current-weapon rune / other weapon / other-weapon rune).
struct REECHO_API FReEchoWeaponSlotOffer
{
	EReEchoShopOfferKind Kind = EReEchoShopOfferKind::Part;
	FName PartId;       // valid when Kind == Part
	FName WeaponId;     // valid when Kind == Weapon
	FName ItemId;       // purchase lookup key (== PartId or WeaponId)
	FName ContentId;    // == PartId or WeaponId
	FName SlotTypeId;
	FText DisplayName;
	FText EffectText;
	int32 Price = 0;
};

// One build-card shop slot offer (one per drop-level tier).
struct REECHO_API FReEchoCardSlotOffer
{
	int32 Tier = 1;
	FName CardId;
	FName ItemId;       // purchase lookup key
	FText DisplayName;
	FText EffectText;
	int32 Price = 0;
	bool bFree = false;
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
	TArray<FReEchoWeaponSlotOffer> SlotOffers;       // fixed 3 slots: [0]=universal rune, [1][2]=weighted (current-weapon rune / other weapon / other-weapon rune)
	TArray<FReEchoCardSlotOffer> CardSlotOffers;     // one per drop-level tier (free tier + shop tiers)
	TArray<FReEchoShopOffer> Offers;                // backward-compat bridge: flatten of SlotOffers + CardSlotOffers (+ whole-weapon as Type==Weapon). TODO(Plan67 Step5): remove once WBP rearranged.
	TArray<FReEchoShopOffer> OwnedParts;
	TArray<FReEchoShopOffer> OwnedCards;
	TArray<FName> OwnedWeapons;
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
