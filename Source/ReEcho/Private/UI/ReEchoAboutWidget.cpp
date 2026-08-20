#include "UI/ReEchoAboutWidget.h"
#include "Components/Button.h"

void UReEchoAboutWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Close)
	{
		Button_Close->OnClicked.AddUniqueDynamic(this, &UReEchoAboutWidget::HandleCloseClicked);
	}
}

void UReEchoAboutWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();
}
