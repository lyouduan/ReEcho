#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReEchoBossPhase3AnnouncementWidget.generated.h"

class SWidget;
class UTextBlock;

/** Non-blocking combat HUD announcement shown once when the hidden Boss Phase3 begins. */
UCLASS()
class REECHO_API UReEchoBossPhase3AnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowAnnouncement();
	static float CalculateAnnouncementOpacity(float ElapsedSeconds,
	                                          float HoldSeconds = 0.75f,
	                                          float FadeSeconds = 1.25f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<UTextBlock> AnnouncementText;

	float ElapsedSeconds = 0.0f;
	bool bPlaying = false;
};
