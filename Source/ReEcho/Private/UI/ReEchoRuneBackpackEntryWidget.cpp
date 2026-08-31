#include "UI/ReEchoRuneBackpackEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/ReEchoIndexedButton.h"

UReEchoRuneBackpackEntryWidget::UReEchoRuneBackpackEntryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

void UReEchoRuneBackpackEntryWidget::ApplyDesignerPreviewSettings()
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
#endif
}

UReEchoIndexedButton* UReEchoRuneBackpackEntryWidget::GetSelectButton() const
{
	return SelectButton;
}

void UReEchoRuneBackpackEntryWidget::Configure(const FReEchoRuneBackpackEntryView& View, const int32 Index)
{
	PendingView = View;
	PendingIndex = Index;
	ApplyContent(PendingView);
	if (SelectButton)
	{
		SelectButton->SetEntryIndex(Index);
		SelectButton->SetIsEnabled(Index != INDEX_NONE);
		if (Index == INDEX_NONE)
		{
			SelectButton->SetToolTip(nullptr);
		}
	}
}

void UReEchoRuneBackpackEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime())
	{
		ApplyContent(DesignerPreview);
	}
	else
	{
		Configure(PendingView, PendingIndex);
	}
}

void UReEchoRuneBackpackEntryWidget::ApplyContent(const FReEchoRuneBackpackEntryView& View)
{
	if (NameText)
	{
		NameText->SetText(View.Name);
	}
	if (CountText)
	{
		CountText->SetText(View.Count > 1 ? FText::Format(NSLOCTEXT("ReEcho", "RuneBackpackCount", "×{0}"),
		                                                  FText::AsNumber(View.Count))
		                                  : FText::GetEmpty());
		CountText->SetVisibility(View.Count > 1 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (RuneIcon)
	{
		// Native texture aspect feeds the authored ScaleToFit; outer SizeBox/slots stay untouched.
		RuneIcon->SetBrushFromTexture(View.Icon, View.Icon != nullptr);
		if (View.Icon)
		{
			// SetBrushFromTexture skips size updates when the resource is unchanged (the first sample).
			// An explicit native aspect also repairs zero ImageSize serialized by the Editor authoring API.
			FSlateBrush Brush = RuneIcon->GetBrush();
			const FIntPoint NativeSize = View.Icon->GetImportedSize();
			Brush.ImageSize = FVector2D(NativeSize.X, NativeSize.Y);
			RuneIcon->SetBrush(Brush);
		}
		RuneIcon->SetVisibility(View.Icon ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}
