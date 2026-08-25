#include "UI/ReEchoIndexedButton.h"

#include "Blueprint/UserWidget.h"
#include "UI/Framework/ReEchoUIInteractionAudit.h"

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
	const UUserWidget* OwnerWidget = GetTypedOuter<UUserWidget>();
	ReEchoUIInteractionAudit::Write(
	    TEXT("BUTTON_CLICK"),
	    FString::Printf(TEXT("screen=DynamicOrRegistered widget=%s button=%s class=%s index=%d"),
	                    OwnerWidget ? *OwnerWidget->GetName() : TEXT("Unknown"),
	                    *GetName(),
	                    *GetClass()->GetName(),
	                    EntryIndex));
	if (EntryIndex != INDEX_NONE)
	{
		OnIndexedClicked.Broadcast(EntryIndex);
	}
}
