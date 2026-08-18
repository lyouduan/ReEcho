#pragma once

#include "Cards/ReEchoCardTypes.h"

class REECHOCARDS_API FReEchoCardCatalog
{
public:
	bool Initialize(const TArray<FReEchoCardDefinition>& Definitions, const FString& DomainRevision, FString& OutError);

	const FReEchoCardDefinition* Find(FName CardId) const;
	TArray<FReEchoCardDefinition> GetOfferable(FName OfferGroup, int32 Tier = INDEX_NONE) const;

	const FString& GetDomainRevision() const
	{
		return Revision;
	}

	int32 Num() const
	{
		return Cards.Num();
	}

	static bool IsSupportedTrigger(FName Trigger);
	static bool IsSupportedBehavior(FName BehaviorId);

private:
	TMap<FName, FReEchoCardDefinition> Cards;
	TArray<FName> CardOrder;
	FString Revision;
};
