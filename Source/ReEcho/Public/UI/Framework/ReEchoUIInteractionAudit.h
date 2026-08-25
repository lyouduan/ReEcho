#pragma once

#include "CoreMinimal.h"
#include "UI/Framework/ReEchoUIScreenTypes.h"

/** Shipping-safe UI interaction audit written to Saved/Logs/UIInteractionAudit.log. */
namespace ReEchoUIInteractionAudit
{
REECHO_API void Write(const FString& Event, const FString& Fields);
REECHO_API FString ScreenName(EReEchoUIScreen Screen);
REECHO_API FString LayerName(EReEchoUILayer Layer);
} // namespace ReEchoUIInteractionAudit
