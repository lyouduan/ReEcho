#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "ReEchoRuntimeAssetPreloader.generated.h"

struct FStreamableHandle;

/** Keeps first-encounter presentation assets resident for the GameInstance lifetime. */
UCLASS()

class REECHO_API UReEchoRuntimeAssetPreloader : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RequestPreload(FSimpleDelegate Completion);

	bool IsPreloadComplete() const
	{
		return bPreloadComplete;
	}

	bool DidPreloadSucceed() const
	{
		return bPreloadSucceeded;
	}

	const TArray<FSoftObjectPath>& GetPreloadAssetPaths() const
	{
		return PreloadAssetPaths;
	}

	static TArray<FSoftObjectPath> BuildDefaultAssetList();
	static TArray<FSoftObjectPath> NormalizeAssetPaths(const TArray<FString>& RawPaths);

#if WITH_DEV_AUTOMATION_TESTS
	void BeginPreloadForTests(const TArray<FString>& RawPaths);
	void CompletePreloadForTests(bool bSucceeded);
#endif

private:
	void BeginPreload(const TArray<FSoftObjectPath>& AssetPaths);
	void HandleAsyncLoadComplete();
	void FinishPreload(bool bSucceeded);

	TArray<FSoftObjectPath> PreloadAssetPaths;
	TArray<FSimpleDelegate> CompletionCallbacks;
	TSharedPtr<FStreamableHandle> StreamableHandle;
	bool bPreloadStarted = false;
	bool bPreloadComplete = false;
	bool bPreloadSucceeded = false;
};
