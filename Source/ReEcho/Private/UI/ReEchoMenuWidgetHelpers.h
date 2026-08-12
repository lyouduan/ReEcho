#pragma once

#include "CoreMinimal.h"

class UButton;
class UReEchoIndexedButton;
class UVerticalBox;
class UWidgetTree;

namespace ReEcho::UI
{
struct FMenuButtonStyle
{
	FLinearColor Color = FLinearColor::White;
	FMargin SlotPadding;
	FMargin LabelMargin;
	int32 FontSize = 24;
};

UButton* AddMenuButton(UWidgetTree& WidgetTree,
                       UVerticalBox& Content,
                       FName ButtonName,
                       const FText& Label,
                       const FMenuButtonStyle& Style);

UReEchoIndexedButton* AddIndexedMenuButton(UWidgetTree& WidgetTree,
                                           UVerticalBox& Content,
                                           FName ButtonName,
                                           const FText& Label,
                                           int32 EntryIndex,
                                           const FMenuButtonStyle& Style);
}
