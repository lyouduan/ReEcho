#include "UI/Framework/ReEchoButtonVisualFeedback.h"

#include "Components/Button.h"
#include "Components/Widget.h"

namespace ReEcho::UI
{
constexpr float ButtonHoverScale = 1.05f;
const FVector2D ButtonHoverPivot(0.5f, 0.5f);
}

void UReEchoButtonVisualFeedback::Bind(UButton* InButton, UWidget* InVisualRoot)
{
	if (!InButton || !InVisualRoot || Button.Get() == InButton)
	{
		return;
	}

	Button = InButton;
	VisualRoot = InVisualRoot;
	RestingScale = InVisualRoot->GetRenderTransform().Scale;
	RestingPivot = InVisualRoot->GetRenderTransformPivot();
	InButton->OnHovered.AddUniqueDynamic(this, &UReEchoButtonVisualFeedback::HandleHovered);
	InButton->OnUnhovered.AddUniqueDynamic(this, &UReEchoButtonVisualFeedback::HandleUnhovered);
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
