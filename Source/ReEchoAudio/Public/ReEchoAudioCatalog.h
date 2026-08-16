#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "ReEchoAudioTypes.h"

struct FStreamableHandle;

/** Catalog provider seam consumed by the policy engine. */
class REECHOAUDIO_API IReEchoAudioCatalogProvider
{
public:
	virtual ~IReEchoAudioCatalogProvider() = default;
	virtual const FReEchoAudioEventDefinition* FindDefinition(FName EventId) const = 0;
};

enum class EReEchoAudioCatalogPreloadState : uint8
{
	NotStarted,
	Loading,
	Ready,
	Failed
};

/**
 * Atomic, data-driven audio event catalog.
 *
 * CSV is parsed and validated into temporary storage. The active catalog is
 * replaced only after every row succeeds, so a bad reload preserves the last
 * known-good definitions. Asset references remain soft and missing assets are
 * safe no-ops at playback time.
 */
class REECHOAUDIO_API FReEchoAudioCatalog : public IReEchoAudioCatalogProvider
{
public:
	void AddDefinition(const FReEchoAudioEventDefinition& Definition);
	void Clear();
	virtual const FReEchoAudioEventDefinition* FindDefinition(FName EventId) const override;

	/** Atomically load the locked Plan34 CSV schema. Returns false without mutating the active catalog on failure. */
	bool LoadCatalog(const FString& CsvPath);

	/** Start or retry asynchronous preload of all non-empty soft references. */
	void PreloadSoftAssets(FStreamableManager& StreamableManager);
	void CancelPreload();

	EReEchoAudioCatalogPreloadState GetPreloadState() const { return PreloadState; }
	bool AreSoftAssetsPreloaded() const { return PreloadState == EReEchoAudioCatalogPreloadState::Ready; }
	const TArray<FSoftObjectPath>& GetSoftAssetPaths() const { return SoftAssetPaths; }
	const FString& GetLastLoadError() const { return LastLoadError; }
	int32 Num() const { return Definitions.Num(); }

private:
	void HandlePreloadComplete();
	bool FailLoad(const FString& Message);

	TMap<FName, FReEchoAudioEventDefinition> Definitions;
	TArray<FSoftObjectPath> SoftAssetPaths;
	TSharedPtr<FStreamableHandle> PreloadHandle;
	EReEchoAudioCatalogPreloadState PreloadState = EReEchoAudioCatalogPreloadState::NotStarted;
	FString LastLoadError;
};
