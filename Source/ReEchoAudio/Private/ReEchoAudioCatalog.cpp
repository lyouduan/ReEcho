#include "ReEchoAudioCatalog.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "ReEchoAudioDataAsset.h"


void FReEchoAudioCatalog::AddDefinition(const FReEchoAudioEventDefinition& Definition)
{
	Definitions.Add({Definition.EventId, Definition.VariantId}, Definition);
}

void FReEchoAudioCatalog::Clear()
{
	CancelPreload();
	Definitions.Empty();
	SoftAssetPaths.Reset();
	LastLoadError.Reset();
}

bool FReEchoAudioCatalog::LoadCatalog(const UReEchoAudioDataAsset& DataAsset)
{
	TMap<FReEchoAudioCatalogKey, FReEchoAudioEventDefinition> PendingDefinitions;
	TArray<FSoftObjectPath> PendingAssetPaths;
	for (int32 Index = 0; Index < DataAsset.Events.Num(); ++Index)
	{
		const FReEchoAudioEventDefinition& Definition = DataAsset.Events[Index];
		auto FailRow = [&](const TCHAR* Field, const TCHAR* Message)
		{
			return FailLoad(FString::Printf(TEXT("%s[%d].%s: %s; preserving previous catalog"),
			                                *DataAsset.GetPathName(),
			                                Index,
			                                Field,
			                                Message));
		};

		if (Definition.EventId.IsNone())
		{
			return FailRow(TEXT("EventId"), TEXT("required value is empty"));
		}
		const FReEchoAudioCatalogKey Key{Definition.EventId, Definition.VariantId};
		if (PendingDefinitions.Contains(Key))
		{
			return FailRow(TEXT("VariantId"), TEXT("duplicate event/variant pair"));
		}
		if (Definition.BaseVolume < 0.0f || Definition.BaseVolume > 1.0f)
		{
			return FailRow(TEXT("BaseVolume"), TEXT("must be in [0,1]"));
		}
		if (Definition.PitchMin <= 0.0f || Definition.PitchMax < Definition.PitchMin)
		{
			return FailRow(TEXT("Pitch"), TEXT("PitchMin must be > 0 and PitchMax must be >= PitchMin"));
		}
		if (Definition.CooldownSeconds < 0.0f || Definition.MaxConcurrency < 0 || Definition.Priority < 0 ||
		    Definition.StartTimeSeconds < 0.0f || Definition.AttenuationMin < 0.0f ||
		    Definition.AttenuationMax < Definition.AttenuationMin)
		{
			return FailRow(TEXT("Ranges"), TEXT("one or more numeric values are outside the supported range"));
		}
		if (!Definition.Sound.IsNull())
		{
			const FSoftObjectPath Path = Definition.Sound.ToSoftObjectPath();
			if (!Path.IsValid())
			{
				return FailRow(TEXT("Sound"), TEXT("invalid Unreal soft object path"));
			}
			PendingAssetPaths.AddUnique(Path);
		}
		PendingDefinitions.Add(Key, Definition);
	}

	if (PendingDefinitions.IsEmpty())
	{
		return FailLoad(FString::Printf(TEXT("%s contains no audio definitions"), *DataAsset.GetPathName()));
	}
	CancelPreload();
	Definitions = MoveTemp(PendingDefinitions);
	SoftAssetPaths = MoveTemp(PendingAssetPaths);
	LastLoadError.Reset();
	UE_LOG(LogReEchoAudio,
	       Log,
	       TEXT("Loaded %d audio definitions atomically from %s"),
	       Definitions.Num(),
	       *DataAsset.GetPathName());
	return true;
}

void FReEchoAudioCatalog::GetAllDefinitions(TArray<FReEchoAudioEventDefinition>& OutDefinitions) const
{
	OutDefinitions.Reset(Definitions.Num());
	for (const TPair<FReEchoAudioCatalogKey, FReEchoAudioEventDefinition>& Pair : Definitions)
	{
		OutDefinitions.Add(Pair.Value);
	}
	OutDefinitions.Sort([](const FReEchoAudioEventDefinition& Left, const FReEchoAudioEventDefinition& Right)
	{
		const int32 EventCompare = Left.EventId.ToString().Compare(Right.EventId.ToString());
		return EventCompare == 0 ? Left.VariantId.ToString() < Right.VariantId.ToString() : EventCompare < 0;
	});
}

const FReEchoAudioEventDefinition* FReEchoAudioCatalog::FindDefinition(const FName EventId, const FName VariantId) const
{
	if (!VariantId.IsNone())
	{
		if (const FReEchoAudioEventDefinition* Exact = Definitions.Find({EventId, VariantId}))
		{
			return Exact;
		}
	}
	return Definitions.Find({EventId, NAME_None});
}

bool FReEchoAudioCatalog::FailLoad(const FString& Message)
{
	LastLoadError = Message;
	UE_LOG(LogReEchoAudio, Error, TEXT("FReEchoAudioCatalog::LoadCatalog: %s"), *Message);
	return false;
}

void FReEchoAudioCatalog::PreloadSoftAssets(FStreamableManager& StreamableManager)
{
	CancelPreload();
	if (SoftAssetPaths.IsEmpty())
	{
		PreloadState = EReEchoAudioCatalogPreloadState::Ready;
		return;
	}
	PreloadState = EReEchoAudioCatalogPreloadState::Loading;
	PreloadHandle = StreamableManager.RequestAsyncLoad(
	    SoftAssetPaths,
	    FStreamableDelegate::CreateRaw(this, &FReEchoAudioCatalog::HandlePreloadComplete),
	    FStreamableManager::AsyncLoadHighPriority);
	if (!PreloadHandle.IsValid())
	{
		PreloadState = EReEchoAudioCatalogPreloadState::Failed;
	}
}

void FReEchoAudioCatalog::HandlePreloadComplete()
{
	for (const FSoftObjectPath& Path : SoftAssetPaths)
	{
		if (Path.ResolveObject() == nullptr)
		{
			PreloadState = EReEchoAudioCatalogPreloadState::Failed;
			UE_LOG(LogReEchoAudio,
			       Warning,
			       TEXT("Audio preload incomplete; missing asset %s (retry remains available)."),
			       *Path.ToString());
			return;
		}
	}
	PreloadState = EReEchoAudioCatalogPreloadState::Ready;
}

void FReEchoAudioCatalog::CancelPreload()
{
	if (PreloadHandle.IsValid() && !PreloadHandle->HasLoadCompleted())
	{
		PreloadHandle->CancelHandle();
	}
	PreloadHandle.Reset();
	PreloadState = EReEchoAudioCatalogPreloadState::NotStarted;
}
