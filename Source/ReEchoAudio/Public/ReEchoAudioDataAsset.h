#pragma once

#include "Engine/DataAsset.h"
#include "ReEchoAudioTypes.h"
#include "ReEchoAudioDataAsset.generated.h"

UCLASS(BlueprintType)
class REECHOAUDIO_API UReEchoAudioDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "音频事件")
	TArray<FReEchoAudioEventDefinition> Events;
};
