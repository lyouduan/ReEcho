#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"

namespace
{
const TCHAR* ResolveSizedElementPath(const TCHAR* Element, const FName TargetId)
{
	const bool bFox = TargetId == TEXT("Enemy.Fox");
	const bool bRabbit = TargetId == TEXT("Enemy.Rabbit");
	const bool bGoat = TargetId == TEXT("Enemy.TimeGuard");
	if (FCString::Stricmp(Element, TEXT("Fire")) == 0)
	{
		return bFox      ? TEXT("/Game/VFX/Element/Fire/Particle/NS_Element_Fire_Fox.NS_Element_Fire_Fox")
		       : bRabbit ? TEXT("/Game/VFX/Element/Fire/Particle/NS_Element_Fire_Rabbit.NS_Element_Fire_Rabbit")
		       : bGoat   ? TEXT("/Game/VFX/Element/Fire/Particle/NS_Element_Fire_Goat.NS_Element_Fire_Goat")
		                 : TEXT("/Game/VFX/Element/Fire/Particle/NS_Element_Fire_ShiLaiMu.NS_Element_Fire_ShiLaiMu");
	}
	return bFox      ? TEXT("/Game/VFX/Element/Water/Particle/NS_Element_Water_Fox.NS_Element_Water_Fox")
	       : bRabbit ? TEXT("/Game/VFX/Element/Water/Particle/NS_Element_Water_Rabbit.NS_Element_Water_Rabbit")
	       : bGoat   ? TEXT("/Game/VFX/Element/Water/Particle/NS_Element_Water_Goat.NS_Element_Water_Goat")
	                 : TEXT("/Game/VFX/Element/Water/Particle/NS_Element_Water1.NS_Element_Water1");
}

}

FReEchoElementReactionVfxPlacement
FReEchoElementReactionVfxCatalog::ResolvePlacement(const EReEchoElementReactionVfxSemantic Semantic)
{
	FReEchoElementReactionVfxPlacement Placement;
	if (Semantic == EReEchoElementReactionVfxSemantic::Burn ||
	    Semantic == EReEchoElementReactionVfxSemantic::Vaporize ||
	    Semantic == EReEchoElementReactionVfxSemantic::Growth ||
	    Semantic == EReEchoElementReactionVfxSemantic::EnhanceGrass ||
	    Semantic == EReEchoElementReactionVfxSemantic::EnhanceWater)
	{
		Placement.bMatchTargetFlipbookSize = true;
	}
	return Placement;
}

const TCHAR* FReEchoElementReactionVfxCatalog::ResolvePath(const EReEchoElementReactionVfxSemantic Semantic,
                                                           const FName TargetId)
{
	switch (Semantic)
	{
		case EReEchoElementReactionVfxSemantic::AttachmentGrass:
		case EReEchoElementReactionVfxSemantic::Growth:
		case EReEchoElementReactionVfxSemantic::EnhanceGrass:
			return TEXT("/Game/VFX/Element/Grass/Particle/NS_Element_Grass.NS_Element_Grass");
		case EReEchoElementReactionVfxSemantic::AttachmentWater:
		case EReEchoElementReactionVfxSemantic::Vaporize:
		case EReEchoElementReactionVfxSemantic::EnhanceWater:
			return ResolveSizedElementPath(TEXT("Water"), TargetId);
		case EReEchoElementReactionVfxSemantic::Burn:
			return ResolveSizedElementPath(TEXT("Fire"), TargetId);
		case EReEchoElementReactionVfxSemantic::Conduct:
			return TEXT("/Game/VFX/Element/Elctricity/Particle/NS_Element_Electricity.NS_Element_Electricity");
		default:
			return TEXT("");
	}
}

void FReEchoElementReactionVfxCatalog::GatherPreloadAssetPaths(TArray<FString>& OutPaths)
{
	OutPaths.Add(ResolvePath(EReEchoElementReactionVfxSemantic::Growth));
	OutPaths.Add(ResolvePath(EReEchoElementReactionVfxSemantic::EnhanceGrass));
	OutPaths.Add(ResolvePath(EReEchoElementReactionVfxSemantic::EnhanceWater));
	OutPaths.Add(ResolvePath(EReEchoElementReactionVfxSemantic::Conduct));
	for (const FName TargetId : {FName(TEXT("Enemy.Slime")),
	                             FName(TEXT("Enemy.Rabbit")),
	                             FName(TEXT("Enemy.Fox")),
	                             FName(TEXT("Enemy.TimeGuard"))})
	{
		OutPaths.Add(ResolvePath(EReEchoElementReactionVfxSemantic::Burn, TargetId));
		OutPaths.Add(ResolvePath(EReEchoElementReactionVfxSemantic::Vaporize, TargetId));
	}
}
