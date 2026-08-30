#include "UI/ReEchoAttributeTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UReEchoAttributeTooltipWidget::UReEchoAttributeTooltipWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoAttributeTooltipWidget::ApplyDesignerPreviewSettings()
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoAttributeTooltipWidget::ConfigureRows(const TArray<FReEchoAttributeRowView>& Rows)
{
	PendingRows = Rows;
	ApplyRows();
}

void UReEchoAttributeTooltipWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (!IsDesignTime())
	{
		ApplyRows();
	}
}

void UReEchoAttributeTooltipWidget::ApplyRows()
{
	if (!AttributeRows || !WidgetTree)
	{
		return;
	}

	TArray<UReEchoAttributeRowWidget*> Rows;
	for (UWidget* Child : AttributeRows->GetAllChildren())
	{
		if (UReEchoAttributeRowWidget* Row = Cast<UReEchoAttributeRowWidget>(Child))
		{
			Rows.Add(Row);
		}
	}

	while (Rows.Num() < PendingRows.Num() && AttributeRowWidgetClass)
	{
		UReEchoAttributeRowWidget* Row =
		    WidgetTree->ConstructWidget<UReEchoAttributeRowWidget>(AttributeRowWidgetClass);
		UVerticalBoxSlot* RowSlot = AttributeRows->AddChildToVerticalBox(Row);
		if (!Rows.IsEmpty())
		{
			// New catalog rows inherit authored slot presentation, never new C++ layout constants.
			if (const UVerticalBoxSlot* TemplateSlot = Cast<UVerticalBoxSlot>(Rows[0]->Slot))
			{
				RowSlot->SetPadding(TemplateSlot->GetPadding());
				RowSlot->SetSize(TemplateSlot->GetSize());
				RowSlot->SetHorizontalAlignment(TemplateSlot->GetHorizontalAlignment());
				RowSlot->SetVerticalAlignment(TemplateSlot->GetVerticalAlignment());
			}
		}
		Rows.Add(Row);
	}

	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const bool bHasData = PendingRows.IsValidIndex(Index);
		Rows[Index]->Configure(bHasData ? PendingRows[Index] : FReEchoAttributeRowView());
		Rows[Index]->SetVisibility(bHasData ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
