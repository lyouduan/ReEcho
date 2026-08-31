#include "Run/ReEchoRuneInventory.h"

#include "Data/ReEchoCsvDataRegistry.h"

namespace ReEchoRuneInventory
{
bool OwnsTerminalTier(const FReEchoCsvDataSnapshot& Snapshot, const FName PartId, const TArray<FName>& OwnedIds)
{
	FName Cursor = PartId;
	TSet<FName> Visited;
	while (const FReEchoCsvRuneUpgradeRow* Rule = Snapshot.RuneUpgrades.Find(Cursor))
	{
		if (Visited.Contains(Cursor))
		{
			return true; // Malformed recipe cycles must never permit endless purchases.
		}
		Visited.Add(Cursor);
		Cursor = Rule->ToPartId;
	}
	return Cursor != PartId && OwnedIds.Contains(Cursor);
}

bool TrySettle(const FReEchoCsvDataSnapshot& Snapshot,
               const FState& Input,
               const FName AcquiredId,
               FState& OutState,
               FString& OutError)
{
	FState Pending = Input;
	TArray<FName> Recipes;
	Snapshot.RuneUpgrades.GetKeys(Recipes);
	Recipes.Sort(
	    [](const FName A, const FName B)
	    {
		    return A.LexicalLess(B);
	    });
	for (const FName FromId : Recipes)
	{
		const FReEchoCsvRuneUpgradeRow& Rule = Snapshot.RuneUpgrades[FromId];
		const FReEchoCsvPartRow* From = Snapshot.Parts.Find(FromId);
		const FReEchoCsvPartRow* To = Snapshot.Parts.Find(Rule.ToPartId);
		// Exact adjacent tiers guarantee termination and forbid cross-family consumption.
		const FString ExpectedTo = FromId.ToString() + TEXT("I");
		const bool bAdjacentTier = FromId.ToString().EndsWith(TEXT("_I")) || FromId.ToString().EndsWith(TEXT("_II"));
		if (Rule.NeedCount != 2 || !From || !To || !From->bEnabled || !To->bEnabled ||
		    From->SlotTypeId != To->SlotTypeId || From->WeaponTypeId != To->WeaponTypeId || !bAdjacentTier ||
		    Rule.ToPartId.ToString() != ExpectedTo)
		{
			OutError = FString::Printf(TEXT("Invalid rune synthesis recipe: %s"), *FromId.ToString());
			return false;
		}
	}
	for (const TPair<FName, int32>& Count : Pending.Counts)
	{
		const FReEchoCsvPartRow* Part = Snapshot.Parts.Find(Count.Key);
		if (Count.Value <= 0 || !Part || !Part->bEnabled)
		{
			OutError = FString::Printf(TEXT("Invalid rune copy count: %s"), *Count.Key.ToString());
			return false;
		}
	}
	TMap<FName, int32> EquippedCounts;
	for (const FName Id : Pending.EquippedIds)
	{
		if (++EquippedCounts.FindOrAdd(Id) > Pending.Counts.FindRef(Id))
		{
			OutError = TEXT("Equipped runes exceed owned copies");
			return false;
		}
	}
	const auto EquippedCount = [&](const FName Id)
	{
		return Pending.EquippedIds
		    .FilterByPredicate(
		        [&](const FName Eq)
		        {
			        return Eq == Id;
		        })
		    .Num();
	};
	const auto BackpackCount = [&](const FName Id)
	{
		return Pending.Counts.FindRef(Id) - EquippedCount(Id);
	};
	const auto ConsumePairs = [&](const FName Id, const int32 Pairs)
	{
		const FName ToId = Snapshot.RuneUpgrades[Id].ToPartId;
		const int32 Remaining = Pending.Counts.FindRef(Id) - Pairs * 2;
		if (Remaining == 0)
		{
			Pending.Counts.Remove(Id);
		}
		else
		{
			Pending.Counts.Add(Id, Remaining);
		}
		const int64 UpgradedCount = static_cast<int64>(Pending.Counts.FindRef(ToId)) + Pairs;
		if (UpgradedCount > MAX_int32)
		{
			OutError = TEXT("Rune synthesis copy count overflow");
			return false;
		}
		Pending.Counts.Add(ToId, static_cast<int32>(UpgradedCount));
		return true;
	};
	// A just-bought copy first upgrades its equipped counterpart, without choosing a different slot.
	const int32 DirectSlot = Pending.EquippedIds.Find(AcquiredId);
	if (DirectSlot != INDEX_NONE && Snapshot.RuneUpgrades.Contains(AcquiredId) && BackpackCount(AcquiredId) > 0)
	{
		if (!ConsumePairs(AcquiredId, 1))
		{
			return false;
		}
		Pending.EquippedIds[DirectSlot] = Snapshot.RuneUpgrades[AcquiredId].ToPartId;
	}
	bool bChanged;
	do
	{
		// Finish every backpack-only cascade before touching any equipped copy.
		bool bBackpackChanged;
		do
		{
			bBackpackChanged = false;
			for (const FName Id : Recipes)
			{
				const int32 Pairs = BackpackCount(Id) / 2;
				if (Pairs > 0)
				{
					if (!ConsumePairs(Id, Pairs))
					{
						return false;
					}
					bBackpackChanged = true;
				}
			}
		} while (bBackpackChanged);

		bChanged = false;
		for (int32 Index = 0; Index < Pending.EquippedIds.Num(); ++Index)
		{
			const FName Id = Pending.EquippedIds[Index];
			const FReEchoCsvRuneUpgradeRow* Rule = Snapshot.RuneUpgrades.Find(Id);
			// An upgrade can meet a same-tier item already in a second slot. Resolve that too before
			// passing the final unique PartId list back to WeaponRuntime.
			int32 DuplicateSlot = INDEX_NONE;
			for (int32 Other = Index + 1; Other < Pending.EquippedIds.Num(); ++Other)
			{
				if (Pending.EquippedIds[Other] == Id)
				{
					DuplicateSlot = Other;
					break;
				}
			}
			if (Rule && (BackpackCount(Id) > 0 || DuplicateSlot != INDEX_NONE))
			{
				const bool bUseDuplicate = BackpackCount(Id) == 0;
				if (!ConsumePairs(Id, 1))
				{
					return false;
				}
				Pending.EquippedIds[Index] = Rule->ToPartId;
				if (bUseDuplicate)
				{
					Pending.EquippedIds.RemoveAt(DuplicateSlot);
				}
				bChanged = true;
			}
			else if (!Rule && DuplicateSlot != INDEX_NONE)
			{
				// Terminal duplicates are retained as backpack copies, never silently destroyed.
				Pending.EquippedIds.RemoveAt(DuplicateSlot);
				bChanged = true;
			}
		}
	} while (bChanged);
	OutState = MoveTemp(Pending);
	OutError.Reset();
	return true;
}
}
