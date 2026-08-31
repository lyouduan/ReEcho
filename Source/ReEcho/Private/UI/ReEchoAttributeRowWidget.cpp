#include "UI/ReEchoAttributeRowWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

UReEchoAttributeRowWidget::UReEchoAttributeRowWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoAttributeRowWidget::ApplyDesignerPreviewSettings()
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoAttributeRowWidget::Configure(const FReEchoAttributeRowView& Row)
{
	PendingRow = Row;
	ApplyContent(PendingRow);
}

void UReEchoAttributeRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyContent(IsDesignTime() ? DesignerPreview : PendingRow);
}

void UReEchoAttributeRowWidget::ApplyContent(const FReEchoAttributeRowView& Row)
{
	if (AttributeNameText)
	{
		AttributeNameText->SetText(Row.Name);
	}
	if (AttributeValueText)
	{
		AttributeValueText->SetText(Row.Value);
	}
	if (AttributeIcon)
	{
		// Preserve the authored Brush dimensions, tint, draw mode and ScaleBox geometry.
		AttributeIcon->SetBrushResourceObject(Row.Icon);
	}
}
