#include "UI/ReEchoLoadoutEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/ReEchoIndexedButton.h"

void UReEchoLoadoutEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoLoadoutEntryWidget::HandleIndexedClicked);
	SelectButton->OnHovered.AddUniqueDynamic(this, &UReEchoLoadoutEntryWidget::HandleHovered);
}

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
	NameText->SetColorAndOpacity(
	    FSlateColor(!bHasPreview || bIsPreviewed ? FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)
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
	USizeBox* TooltipSize = WidgetTree->ConstructWidget<USizeBox>();
	TooltipSize->SetWidthOverride(380.0f);

	UBorder* TooltipFrame = WidgetTree->ConstructWidget<UBorder>();
	TooltipFrame->SetBrushColor(FLinearColor(0.95f, 0.88f, 0.72f, 1.0f));
	TooltipFrame->SetPadding(FMargin(3.0f));
	UBorder* TooltipSurface = WidgetTree->ConstructWidget<UBorder>();
	TooltipSurface->SetBrushColor(FLinearColor(0.015f, 0.015f, 0.015f, 0.97f));
	TooltipSurface->SetPadding(FMargin(18.0f, 14.0f));

	UVerticalBox* TooltipContent = WidgetTree->ConstructWidget<UVerticalBox>();
	UTextBlock* TooltipTitle = WidgetTree->ConstructWidget<UTextBlock>();
	TooltipTitle->SetText(Label);
	TooltipTitle->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.96f, 0.88f, 1.0f)));
	TooltipTitle->SetJustification(ETextJustify::Center);
	TooltipTitle->SetAutoWrapText(false);
	FSlateFontInfo TitleFont = TooltipTitle->GetFont();
	TitleFont.Size = 22;
	TooltipTitle->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = TooltipContent->AddChildToVerticalBox(TooltipTitle);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	UTextBlock* TooltipBody = WidgetTree->ConstructWidget<UTextBlock>();
	TooltipBody->SetText(Description);
	TooltipBody->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TooltipBody->SetJustification(ETextJustify::Left);
	TooltipBody->SetAutoWrapText(true);
	TooltipBody->SetWrapTextAt(338.0f);
	FSlateFontInfo BodyFont = TooltipBody->GetFont();
	BodyFont.Size = 20;
	TooltipBody->SetFont(BodyFont);
	TooltipContent->AddChildToVerticalBox(TooltipBody);

	TooltipSurface->SetContent(TooltipContent);
	TooltipFrame->SetContent(TooltipSurface);
	TooltipSize->SetContent(TooltipFrame);
	return TooltipSize;
}

void UReEchoLoadoutEntryWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	if (EntryIndex != INDEX_NONE && (!SelectButton || !SelectButton->IsHovered()))
	{
		OnEntryPreviewed.Broadcast(EntryIndex);
	}
}
