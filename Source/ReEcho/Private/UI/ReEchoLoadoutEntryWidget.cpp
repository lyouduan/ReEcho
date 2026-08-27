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
	static ConstructorHelpers::FClassFinder<UReEchoLoadoutTooltipWidget> TooltipClassFinder(
	    TEXT("/Game/ReEcho/UI/WBP_ReEchoLoadoutTooltip"));
	TooltipWidgetClass = TooltipClassFinder.Class;
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
	ApplyDesignerPreviewState(bDesignerPreviewSelected);
#endif
}

#if WITH_EDITOR
void UReEchoLoadoutEntryWidget::ApplyDesignerPreviewState(const bool bIsSelected)
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
		PortraitImage->SetBrushFromTexture(Texture, true);
	}
	if (NameText)
	{
		NameText->SetColorAndOpacity(FSlateColor(bIsSelected ? FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)
		                                                     : FLinearColor(0.46f, 0.43f, 0.37f, 1.0f)));
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
		PortraitImage->SetBrushFromTexture(Texture, true);
	}
	NameText->SetColorAndOpacity(FSlateColor(!bHasPreview || bIsPreviewed ? FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)
	                                                                      : FLinearColor(0.46f, 0.43f, 0.37f, 1.0f)));
	SelectButton->SetBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
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
