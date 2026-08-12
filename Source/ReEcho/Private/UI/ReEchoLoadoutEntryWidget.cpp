#include "UI/ReEchoLoadoutEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/ReEchoIndexedButton.h"

namespace
{
const FLinearColor SelectedColor(0.08f, 0.52f, 0.34f, 1.0f);
const FLinearColor UnselectedColor(0.10f, 0.18f, 0.30f, 1.0f);
}

void UReEchoLoadoutEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SelectButton->OnIndexedClicked.AddUniqueDynamic(this, &UReEchoLoadoutEntryWidget::HandleIndexedClicked);
}

void UReEchoLoadoutEntryWidget::Configure(const int32 InEntryIndex,
                                          const FText& Label,
                                          const FString& TexturePath)
{
	EntryIndex = InEntryIndex;
	SelectButton->SetEntryIndex(EntryIndex);
	NameText->SetText(Label);
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath))
	{
		PortraitImage->SetBrushFromTexture(Texture, true);
	}
}

void UReEchoLoadoutEntryWidget::SetSelected(const bool bSelected)
{
	SelectButton->SetBackgroundColor(bSelected ? SelectedColor : UnselectedColor);
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
