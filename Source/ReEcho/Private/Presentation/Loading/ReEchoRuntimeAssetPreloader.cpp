#include "Presentation/Loading/ReEchoRuntimeAssetPreloader.h"

#include "ReEcho.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

void UReEchoRuntimeAssetPreloader::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BeginPreload(BuildDefaultAssetList());
}

void UReEchoRuntimeAssetPreloader::Deinitialize()
{
	CompletionCallbacks.Reset();
	StreamableHandle.Reset();
	PreloadAssetPaths.Reset();
	Super::Deinitialize();
}

void UReEchoRuntimeAssetPreloader::RequestPreload(FSimpleDelegate Completion)
{
	if (bPreloadComplete)
	{
		Completion.ExecuteIfBound();
		return;
	}
	if (Completion.IsBound())
	{
		CompletionCallbacks.Add(MoveTemp(Completion));
	}
	if (!bPreloadStarted)
	{
		BeginPreload(BuildDefaultAssetList());
	}
}

TArray<FSoftObjectPath> UReEchoRuntimeAssetPreloader::BuildDefaultAssetList()
{
	TArray<FString> RawPaths;
	FReEchoCombatVfxCatalog::GatherPreloadAssetPaths(RawPaths);
	FReEchoWeaponVisualCatalog::GatherPreloadAssetPaths(RawPaths);
	return NormalizeAssetPaths(RawPaths);
}

TArray<FSoftObjectPath> UReEchoRuntimeAssetPreloader::NormalizeAssetPaths(const TArray<FString>& RawPaths)
{
	TArray<FSoftObjectPath> Result;
	TSet<FSoftObjectPath> SeenPaths;
	for (FString RawPath : RawPaths)
	{
		RawPath.TrimStartAndEndInline();
		const FSoftObjectPath AssetPath(RawPath);
		if (RawPath.IsEmpty() || !AssetPath.IsValid() || SeenPaths.Contains(AssetPath))
		{
			continue;
		}
		SeenPaths.Add(AssetPath);
		Result.Add(AssetPath);
	}
	return Result;
}

void UReEchoRuntimeAssetPreloader::BeginPreload(const TArray<FSoftObjectPath>& AssetPaths)
{
	if (bPreloadStarted)
	{
		return;
	}
	bPreloadStarted = true;
	PreloadAssetPaths = AssetPaths;
	const double StartSeconds = FPlatformTime::Seconds();
	if (PreloadAssetPaths.IsEmpty())
	{
		UE_LOG(LogReEcho, Display, TEXT("[RuntimeAssetPreload] Empty list completed immediately."));
		FinishPreload(true);
		return;
	}
	StreamableHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
	    PreloadAssetPaths,
	    FStreamableDelegate::CreateUObject(this, &UReEchoRuntimeAssetPreloader::HandleAsyncLoadComplete),
	    FStreamableManager::AsyncLoadHighPriority,
	    false,
	    false,
	    TEXT("ReEchoFirstEncounterPresentation"));
	if (!StreamableHandle.IsValid())
	{
		UE_LOG(LogReEcho, Warning, TEXT("[RuntimeAssetPreload] Request could not be created; gameplay will continue."));
		FinishPreload(false);
		return;
	}
	UE_LOG(LogReEcho,
	       Display,
	       TEXT("[RuntimeAssetPreload] Requested %d assets in %.2f ms."),
	       PreloadAssetPaths.Num(),
	       (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
}

void UReEchoRuntimeAssetPreloader::HandleAsyncLoadComplete()
{
	bool bAllLoaded = true;
	for (const FSoftObjectPath& AssetPath : PreloadAssetPaths)
	{
		if (!AssetPath.ResolveObject())
		{
			bAllLoaded = false;
			UE_LOG(LogReEcho,
			       Warning,
			       TEXT("[RuntimeAssetPreload] Missing optional presentation asset %s."),
			       *AssetPath.ToString());
		}
	}
	FinishPreload(bAllLoaded);
}

void UReEchoRuntimeAssetPreloader::FinishPreload(const bool bSucceeded)
{
	if (bPreloadComplete)
	{
		return;
	}
	bPreloadComplete = true;
	bPreloadSucceeded = bSucceeded;
	UE_LOG(LogReEcho,
	       Display,
	       TEXT("[RuntimeAssetPreload] Completed. Success=%s ResidentAssets=%d"),
	       bSucceeded ? TEXT("true") : TEXT("false"),
	       PreloadAssetPaths.Num());
	TArray<FSimpleDelegate> Callbacks = MoveTemp(CompletionCallbacks);
	CompletionCallbacks.Reset();
	for (FSimpleDelegate& Callback : Callbacks)
	{
		Callback.ExecuteIfBound();
	}
}

#if WITH_DEV_AUTOMATION_TESTS
void UReEchoRuntimeAssetPreloader::BeginPreloadForTests(const TArray<FString>& RawPaths)
{
	BeginPreload(NormalizeAssetPaths(RawPaths));
}

void UReEchoRuntimeAssetPreloader::CompletePreloadForTests(const bool bSucceeded)
{
	FinishPreload(bSucceeded);
}
#endif
