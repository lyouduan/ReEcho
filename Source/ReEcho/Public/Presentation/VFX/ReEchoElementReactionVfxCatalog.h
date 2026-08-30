#pragma once

#include "CoreMinimal.h"

enum class EReEchoElementReactionVfxSemantic : uint8
{
	AttachmentGrass,
	AttachmentWater,
	Burn,
	Vaporize,
	Growth,
	Conduct,
	EnhanceGrass,
	EnhanceWater
};

struct FReEchoElementReactionVfxPlacement
{
	FVector WorldScale = FVector::OneVector;
	bool bPreserveWorldSize = true;
	bool bMatchTargetFlipbookSize = false;
	float TargetCoverageRatio = 1.0f;
};

/** Presentation-only semantic map and world-size contract for reaction Niagara. */
struct REECHO_API FReEchoElementReactionVfxCatalog
{
	static const TCHAR* ResolvePath(EReEchoElementReactionVfxSemantic Semantic, FName TargetId = NAME_None);
	static FReEchoElementReactionVfxPlacement ResolvePlacement(EReEchoElementReactionVfxSemantic Semantic);
	static void GatherPreloadAssetPaths(TArray<FString>& OutPaths);
};
