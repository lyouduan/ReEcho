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

/** One authoritative result vocabulary for every shop purchase attempt. */
enum class EReEchoShopPurchaseResult : uint8
{
	Succeeded,
	OfferNotFound,
	AlreadyOwned,
	PurchaseDisabled,
	InsufficientCurrency,
	DataUnavailable,
	GrantRejected,
	MutationRejected,
	WeaponSelectionRejected,
	ReplayUnlockRejected
};

/** Structured result returned by the unified purchase transaction interface. */
struct REECHO_API FReEchoShopPurchaseOutcome
{
	FString TransactionId;
	FName ItemId;
	EReEchoShopPurchaseResult Result = EReEchoShopPurchaseResult::OfferNotFound;
	FString Detail;
	int32 EffectivePrice = 0;

	bool IsSuccess() const
	{
		return Result == EReEchoShopPurchaseResult::Succeeded;
	}
};

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
	FString IconTexturePath; // 武器 Offer 填对应配图资产路径(ResolveHeldTexturePath)，为空则回退默认卡片图标
	/** Optional resolved runtime result for an owned card. Empty offers retain the legacy single-panel tooltip. */
	FText OutcomeText;
};

// One fixed weapon/part shop slot offer (left = universal rune, mid/right = weighted current-weapon rune / other weapon
// / other-weapon rune).
struct REECHO_API FReEchoWeaponSlotOffer
{
	EReEchoShopOfferKind Kind = EReEchoShopOfferKind::Part;
	FName PartId;    // valid when Kind == Part
	FName WeaponId;  // valid when Kind == Weapon
	FName ItemId;    // purchase lookup key (== PartId or WeaponId)
	FName ContentId; // == PartId or WeaponId
	FName SlotTypeId;
	FText DisplayName;
	FText EffectText;
	int32 Price = 0;
};

enum class EReEchoShopCardPackStatus : uint8
{
	NotOffered,
	Available,
	Purchased,
	SoldOut
};

/** One independently priced card inside a fixed-tier shop pack. */
struct REECHO_API FReEchoShopCardChoiceOffer
{
	FName CardId;
	FName ItemId; // purchase lookup key
	FText DisplayName;
	FText EffectText;
	TArray<FName> Tags;
	int32 Tier = 1;
	int32 Price = 0;
	int32 SlotIndex = INDEX_NONE;
	int32 SlotRefreshSequence = 0;
	int32 RemainingRefreshes = 0;
	int32 RefreshCost = 0;
	bool bCanRefresh = false;
};

/** One of the three fixed card-pack entrances. Array index 0/1/2 is always tier 1/2/3. */
struct REECHO_API FReEchoShopCardPackOffer
{
	int32 Tier = 1;
	EReEchoShopCardPackStatus Status = EReEchoShopCardPackStatus::NotOffered;
	FText DisplayName;
	FText StatusText;
	TArray<FReEchoShopCardChoiceOffer> Choices;

	bool IsAvailable() const
	{
		return Status == EReEchoShopCardPackStatus::Available && !Choices.IsEmpty();
	}
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
	FString WeaponIconTexturePath;
	TArray<FReEchoWeaponSlotOffer> SlotOffers; // fixed 3 slots: [0]=universal rune, [1][2]=weighted (current-weapon
	                                           // rune / other weapon / other-weapon rune)
	TArray<FReEchoShopCardPackOffer> CardPackOffers; // always 3 fixed tier packs; candidates are opened on demand
	TArray<FReEchoShopOffer> Offers;                 // backward-compat bridge for weapon/rune offers only
	TArray<FReEchoShopOffer> OwnedParts;
	TArray<FReEchoShopOffer> OwnedCards;
	TArray<FName> OwnedWeapons;
	TArray<FReEchoShopOffer> OwnedWeaponOffers;
	TArray<FReEchoWeaponSlotShopView> Slots;
	TArray<FReEchoEquippedPartSnapshot> EquippedParts;
	int32 WeaponRuneRefreshesRemaining = 0;
	int32 WeaponRuneRefreshCost = 0;
	bool bWeaponRuneRefreshAllowed = false;
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
