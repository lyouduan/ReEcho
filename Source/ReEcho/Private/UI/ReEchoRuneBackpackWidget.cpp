#include "UI/ReEchoRuneBackpackWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UReEchoRuneBackpackWidget::UReEchoRuneBackpackWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoRuneBackpackWidget::ApplyDesignerPreviewSettings()
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

TArray<UReEchoRuneBackpackEntryWidget*> UReEchoRuneBackpackWidget::GetEntryWidgets() const
{
	TArray<UReEchoRuneBackpackEntryWidget*> Entries;
	if (EntryList)
	{
		for (UWidget* Child : EntryList->GetAllChildren())
		{
			if (UReEchoRuneBackpackEntryWidget* Entry = Cast<UReEchoRuneBackpackEntryWidget>(Child))
			{
				Entries.Add(Entry);
			}
		}
	}
	return Entries;
}

void UReEchoRuneBackpackWidget::ConfigureEntries(const TArray<FReEchoRuneBackpackEntryView>& Views)
{
	PendingViews = Views;
	ApplyEntries();
}

void UReEchoRuneBackpackWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!IsDesignTime())
	{
		ApplyEntries();
	}
}

void UReEchoRuneBackpackWidget::ApplyEntries()
{
	if (!EntryList || !WidgetTree)
	{
		return;
	}
	TArray<UReEchoRuneBackpackEntryWidget*> Entries = GetEntryWidgets();
	while (Entries.Num() < PendingViews.Num() && EntryWidgetClass)
	{
		UReEchoRuneBackpackEntryWidget* Entry =
		    WidgetTree->ConstructWidget<UReEchoRuneBackpackEntryWidget>(EntryWidgetClass);
		UVerticalBoxSlot* EntrySlot = EntryList->AddChildToVerticalBox(Entry);
		if (!Entries.IsEmpty())
		{
			if (const UVerticalBoxSlot* TemplateSlot = Cast<UVerticalBoxSlot>(Entries[0]->Slot))
			{
				EntrySlot->SetPadding(TemplateSlot->GetPadding());
				EntrySlot->SetSize(TemplateSlot->GetSize());
				EntrySlot->SetHorizontalAlignment(TemplateSlot->GetHorizontalAlignment());
				EntrySlot->SetVerticalAlignment(TemplateSlot->GetVerticalAlignment());
			}
		}
		Entries.Add(Entry);
	}
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const bool bHasData = PendingViews.IsValidIndex(Index);
		Entries[Index]->Configure(bHasData ? PendingViews[Index] : FReEchoRuneBackpackEntryView(),
		                          bHasData ? Index : INDEX_NONE);
		Entries[Index]->SetVisibility(bHasData ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
