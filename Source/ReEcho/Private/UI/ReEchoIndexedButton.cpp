#include "UI/ReEchoIndexedButton.h"

UReEchoIndexedButton::UReEchoIndexedButton(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	OnClicked.AddUniqueDynamic(this, &UReEchoIndexedButton::HandleClicked);
}

void UReEchoIndexedButton::SetEntryIndex(const int32 InIndex)
{
	EntryIndex = InIndex;
}

void UReEchoIndexedButton::HandleClicked()
{
	if (EntryIndex != INDEX_NONE)
	{
		OnIndexedClicked.Broadcast(EntryIndex);
	}
}
