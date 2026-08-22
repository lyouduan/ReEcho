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

/** Presentation-only semantic map for element attachment and reaction Niagara. */
struct REECHO_API FReEchoElementReactionVfxCatalog
{
	static const TCHAR* ResolvePath(EReEchoElementReactionVfxSemantic Semantic, FName TargetId = NAME_None);
	static void GatherPreloadAssetPaths(TArray<FString>& OutPaths);
};
