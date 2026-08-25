#include "UI/Framework/ReEchoButtonVisualFeedback.h"

#include "Components/Button.h"
#include "Components/Widget.h"
#include "UI/Framework/ReEchoUIInteractionAudit.h"
#include "UI/ReEchoIndexedButton.h"

namespace ReEcho::UI
{
constexpr float ButtonHoverScale = 1.05f;
const FVector2D ButtonHoverPivot(0.5f, 0.5f);
}

void UReEchoButtonVisualFeedback::Bind(UButton* InButton,
                                       UWidget* InVisualRoot,
                                       const FString& InScreenName,
                                       const FString& InWidgetName)
{
	if (!InButton || !InVisualRoot || Button.Get() == InButton)
	{
		return;
	}

	Button = InButton;
	VisualRoot = InVisualRoot;
	ScreenName = InScreenName;
	WidgetName = InWidgetName;
	RestingScale = InVisualRoot->GetRenderTransform().Scale;
	RestingPivot = InVisualRoot->GetRenderTransformPivot();
	InButton->OnHovered.AddUniqueDynamic(this, &UReEchoButtonVisualFeedback::HandleHovered);
	InButton->OnUnhovered.AddUniqueDynamic(this, &UReEchoButtonVisualFeedback::HandleUnhovered);
	InButton->OnClicked.AddUniqueDynamic(this, &UReEchoButtonVisualFeedback::HandleClicked);
}

bool UReEchoButtonVisualFeedback::IsBoundTo(const UButton* InButton) const
{
	return Button.Get() == InButton;
}

bool UReEchoButtonVisualFeedback::HasValidButton() const
{
	return Button.IsValid() && VisualRoot.IsValid();
}

void UReEchoButtonVisualFeedback::HandleHovered()
{
	if (UWidget* Root = VisualRoot.Get())
	{
		Root->SetRenderTransformPivot(ReEcho::UI::ButtonHoverPivot);
		Root->SetRenderScale(RestingScale * ReEcho::UI::ButtonHoverScale);
	}
}

void UReEchoButtonVisualFeedback::HandleUnhovered()
{
	if (UWidget* Root = VisualRoot.Get())
	{
		Root->SetRenderScale(RestingScale);
		Root->SetRenderTransformPivot(RestingPivot);
	}
}

void UReEchoButtonVisualFeedback::HandleClicked()
{
	const UButton* ClickedButton = Button.Get();
	if (!ClickedButton || ClickedButton->IsA<UReEchoIndexedButton>())
	{
		// Indexed buttons self-audit so buttons created after the initial screen-tree scan are covered too.
		return;
	}
	ReEchoUIInteractionAudit::Write(TEXT("BUTTON_CLICK"),
	                                FString::Printf(TEXT("screen=%s widget=%s button=%s class=%s"),
	                                                *ScreenName,
	                                                *WidgetName,
	                                                *ClickedButton->GetName(),
	                                                *ClickedButton->GetClass()->GetName()));
}
