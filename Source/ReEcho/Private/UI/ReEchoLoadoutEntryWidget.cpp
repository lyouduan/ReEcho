#include "UI/ReEchoLoadoutEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/ReEchoIndexedButton.h"
#include "UI/ReEchoLoadoutTooltipWidget.h"
#include "UObject/ConstructorHelpers.h"

UReEchoLoadoutEntryWidget::UReEchoLoadoutEntryWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
	DesignTimeSize = FVector2D(390.0f, 560.0f);
#endif
	static ConstructorHelpers::FClassFinder<UReEchoLoadoutTooltipWidget> TooltipClassFinder(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip"));
	TooltipWidgetClass = TooltipClassFinder.Class;
}

void UReEchoLoadoutEntryWidget::ApplyDesignerPreviewSettings()
{
#if WITH_EDITOR
	Modify();
#endif
#if WITH_EDITORONLY_DATA
	DesignSizeMode = EDesignPreviewSizeMode::Desired;
	DesignTimeSize = FVector2D(390.0f, 560.0f);
#endif
#if WITH_EDITOR
	MarkPackageDirty();
#endif
}

bool UReEchoLoadoutEntryWidget::HasDesiredDesignerPreview() const
{
#if WITH_EDITORONLY_DATA
	return DesignSizeMode == EDesignPreviewSizeMode::Desired;
#else
	return true;
#endif
}

void UReEchoLoadoutEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoLoadoutEntryWidget::HandleIndexedClicked);
	SelectButton->OnHovered.AddUniqueDynamic(this, &UReEchoLoadoutEntryWidget::HandleHovered);
}

void UReEchoLoadoutEntryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
#if WITH_EDITOR
	if (!IsDesignTime())
	{
		return;
	}
	if (NameText && !DesignerPreviewLabel.IsEmpty())
	{
		NameText->SetText(DesignerPreviewLabel);
	}
	if (EntryRootSizeBox)
	{
		EntryRootSizeBox->SetWidthOverride(DesignerPreviewWidth);
		EntryRootSizeBox->SetHeightOverride(DesignerPreviewHeight);
	}
	if (PortraitSize)
	{
		PortraitSize->SetWidthOverride(DesignerPreviewWidth);
		PortraitSize->SetHeightOverride(480.0f);
	}
	RefreshSelectionArrowBrushSize();
	ApplyDesignerPreviewState(bDesignerPreviewSelected, bDesignerPreviewSelected);
#endif
}

#if WITH_EDITOR
void UReEchoLoadoutEntryWidget::ApplyDesignerPreviewState(const bool bIsSelected, const bool bShowSelectionArrow)
{
#if WITH_EDITORONLY_DATA
	bDesignerPreviewSelected = bIsSelected;
	UTexture2D* Texture = bIsSelected ? DesignerPreviewSelectedTexture : DesignerPreviewUnselectedTexture;
	if (!Texture)
	{
		Texture = DesignerPreviewSelectedTexture;
	}
	if (PortraitImage && Texture)
	{
		ApplyPortraitTexture(Texture);
	}
	if (NameText)
	{
		NameText->SetColorAndOpacity(FSlateColor(bIsSelected ? FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)
		                                                     : FLinearColor(0.46f, 0.43f, 0.37f, 1.0f)));
	}
	if (EntrySelectionArrow)
	{
		EntrySelectionArrow->SetVisibility(bShowSelectionArrow ? ESlateVisibility::HitTestInvisible
		                                                             : ESlateVisibility::Collapsed);
	}
#endif
}
#endif

void UReEchoLoadoutEntryWidget::Configure(const int32 InEntryIndex,
                                          const FText& Label,
                                          const FText& Description,
                                          const FString& SelectedTexturePath,
                                          const FString& UnselectedTexturePath,
                                          const FString& FallbackTexturePath,
                                          const float EntryWidth,
                                          const float EntryHeight)
{
	EntryIndex = InEntryIndex;
	SelectButton->SetEntryIndex(EntryIndex);
	NameText->SetText(Label);
	SelectButton->SetToolTip(BuildTooltip(Label, Description));
	if (EntryRootSizeBox)
	{
		EntryRootSizeBox->SetWidthOverride(EntryWidth);
		EntryRootSizeBox->SetHeightOverride(EntryHeight);
	}
	if (PortraitSize)
	{
		PortraitSize->SetWidthOverride(EntryWidth);
		PortraitSize->SetHeightOverride(480.0f);
	}
	SelectedTexture = LoadObject<UTexture2D>(nullptr, *SelectedTexturePath);
	UnselectedTexture = LoadObject<UTexture2D>(nullptr, *UnselectedTexturePath);
	if (!SelectedTexture)
	{
		SelectedTexture = LoadObject<UTexture2D>(nullptr, *FallbackTexturePath);
	}
	if (!UnselectedTexture)
	{
		UnselectedTexture = SelectedTexture;
	}
	SetPresentationState(false, false);
}

void UReEchoLoadoutEntryWidget::SetPresentationState(const bool bHasPreview, const bool bIsPreviewed)
{
	UTexture2D* Texture = !bHasPreview || bIsPreviewed ? SelectedTexture : UnselectedTexture;
	if (Texture)
	{
		ApplyPortraitTexture(Texture);
	}
	NameText->SetColorAndOpacity(FSlateColor(!bHasPreview || bIsPreviewed ? FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)
	                                                                      : FLinearColor(0.46f, 0.43f, 0.37f, 1.0f)));
	if (EntrySelectionArrow)
	{
		RefreshSelectionArrowBrushSize();
		EntrySelectionArrow->SetVisibility(bHasPreview && bIsPreviewed ? ESlateVisibility::HitTestInvisible
		                                                                  : ESlateVisibility::Collapsed);
	}
	SelectButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
}

void UReEchoLoadoutEntryWidget::ApplyPortraitTexture(UTexture2D* Texture)
{
	if (!PortraitImage || !Texture)
	{
		return;
	}

	// SetBrushFromTexture is a no-op when the resource is already assigned, which can leave a stale
	// authored ImageSize behind. ScaleBox aspect fitting depends on that desired size, so always refresh it.
	PortraitImage->SetBrushFromTexture(Texture, true);
	FSlateBrush Brush = PortraitImage->GetBrush();
	Brush.SetResourceObject(Texture);
	const FIntPoint ImportedSize = Texture->GetImportedSize();
	Brush.ImageSize = FVector2D(ImportedSize.X, ImportedSize.Y);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	PortraitImage->SetBrush(Brush);
}

void UReEchoLoadoutEntryWidget::RefreshSelectionArrowBrushSize()
{
	if (!EntrySelectionArrow)
	{
		return;
	}

	FSlateBrush Brush = EntrySelectionArrow->GetBrush();
	if (const UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject()))
	{
		const FIntPoint ImportedSize = Texture->GetImportedSize();
		Brush.ImageSize = FVector2D(ImportedSize.X, ImportedSize.Y);
		EntrySelectionArrow->SetBrush(Brush);
	}
}

void UReEchoLoadoutEntryWidget::FocusSelection()
{
	SelectButton->SetKeyboardFocus();
}

void UReEchoLoadoutEntryWidget::HandleIndexedClicked(const int32 ClickedIndex)
{
	if (ClickedIndex == EntryIndex)
	{
		OnEntrySelected.Broadcast(EntryIndex);
	}
}

void UReEchoLoadoutEntryWidget::HandleHovered()
{
	if (EntryIndex != INDEX_NONE)
	{
		OnEntryHovered.Broadcast(EntryIndex);
	}
}

UWidget* UReEchoLoadoutEntryWidget::BuildTooltip(const FText& Label, const FText& Description)
{
	TSubclassOf<UReEchoLoadoutTooltipWidget> EffectiveTooltipClass = TooltipWidgetClass;
	if (!EffectiveTooltipClass)
	{
		EffectiveTooltipClass = LoadClass<UReEchoLoadoutTooltipWidget>(
		    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip.WBP_ReEchoLoadoutTooltip_C"));
	}
	if (!EffectiveTooltipClass)
	{
		EffectiveTooltipClass = UReEchoLoadoutTooltipWidget::StaticClass();
	}

	UReEchoLoadoutTooltipWidget* Tooltip =
	    WidgetTree->ConstructWidget<UReEchoLoadoutTooltipWidget>(EffectiveTooltipClass);
	Tooltip->Configure(Label, Description);
	return Tooltip;
}

void UReEchoLoadoutEntryWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	if (EntryIndex != INDEX_NONE && (!SelectButton || !SelectButton->IsHovered()))
	{
		OnEntryPreviewed.Broadcast(EntryIndex);
	}
}
