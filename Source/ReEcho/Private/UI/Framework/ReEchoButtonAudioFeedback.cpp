#include "UI/Framework/ReEchoButtonAudioFeedback.h"

#include "Components/Button.h"
#include "Engine/GameInstance.h"
#include "ReEchoAudioService.h"

void UReEchoButtonAudioFeedback::Bind(UButton* InButton,
                                      UGameInstance* InGameInstance,
                                      const FName InHoverEventId,
                                      const FName InClickEventId)
{
	if (!InButton || !InGameInstance)
	{
		return;
	}

	const bool bAlreadyBound = Button.Get() == InButton;
	Button = InButton;
	GameInstance = InGameInstance;
	HoverEventId = InHoverEventId;
	ClickEventId = InClickEventId;
	if (!bAlreadyBound)
	{
		InButton->OnHovered.AddUniqueDynamic(this, &UReEchoButtonAudioFeedback::HandleHovered);
		InButton->OnClicked.AddUniqueDynamic(this, &UReEchoButtonAudioFeedback::HandleClicked);
	}
}

bool UReEchoButtonAudioFeedback::IsBoundTo(const UButton* InButton) const
{
	return Button.Get() == InButton;
}

bool UReEchoButtonAudioFeedback::HasValidButton() const
{
	return Button.IsValid() && GameInstance.IsValid();
}

void UReEchoButtonAudioFeedback::PostEvent(const FName EventId) const
{
	UGameInstance* BoundGameInstance = GameInstance.Get();
	if (EventId.IsNone() || !BoundGameInstance)
	{
		return;
	}
	if (UReEchoAudioService* AudioService = BoundGameInstance->GetSubsystem<UReEchoAudioService>())
	{
		AudioService->PostEventById(BoundGameInstance, EventId);
	}
}

void UReEchoButtonAudioFeedback::HandleHovered()
{
	PostEvent(HoverEventId);
}

void UReEchoButtonAudioFeedback::HandleClicked()
{
	PostEvent(ClickEventId);
}
