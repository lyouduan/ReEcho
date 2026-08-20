#include "UI/ReEchoAboutWidget.h"
#include "Components/Button.h"

void UReEchoAboutWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UReEchoAboutWidget::HandleCloseClicked);
	}
}

void UReEchoAboutWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();
}
