#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Presentation/Loading/ReEchoRuntimeAssetPreloader.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuntimeAssetPreloadCatalogTest,
                                 "ReEcho.Presentation.RuntimeAssetPreload.Catalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuntimeAssetPreloadCatalogTest::RunTest(const FString& Parameters)
{
	const TArray<FSoftObjectPath> AssetPaths = UReEchoRuntimeAssetPreloader::BuildDefaultAssetList();
	TestEqual(TEXT("The preload catalog contains combat, element and weapon presentation assets"),
	          AssetPaths.Num(),
	          31);
	TSet<FSoftObjectPath> UniquePaths;
	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		TestTrue(TEXT("Every preload path is valid"), AssetPath.IsValid());
		TestFalse(TEXT("Every preload path is unique"), UniquePaths.Contains(AssetPath));
		UniquePaths.Add(AssetPath);
	}
	TestTrue(TEXT("Known-safe melee fallback is included"),
	         UniquePaths.Contains(FSoftObjectPath(TEXT("/Game/ReEcho/Textures/Effects/SlashCrescent.SlashCrescent"))));
	TestTrue(TEXT("Missing specialist art is still warmed before its synchronous fallback"),
	         UniquePaths.Contains(FSoftObjectPath(TEXT("/Game/ReEcho/Textures/Effects/WhipLash.WhipLash"))));
	TestTrue(TEXT("Grass attachment Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/Element/Grass/Particle/NS_Element_Grass.NS_Element_Grass"))));
	TestTrue(TEXT("Conduct Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(TEXT(
	             "/Game/VFX/Element/Elctricity/Particle/NS_Element_Electricity.NS_Element_Electricity"))));

	const TArray<FString> DirtyPaths = {TEXT("  /Game/ReEcho/Textures/Effects/Bow.Bow  "),
	                                    TEXT("/Game/ReEcho/Textures/Effects/Bow.Bow"),
	                                    TEXT(""),
	                                    TEXT("   ")};
	const TArray<FSoftObjectPath> Normalized = UReEchoRuntimeAssetPreloader::NormalizeAssetPaths(DirtyPaths);
	TestEqual(TEXT("Normalization trims, removes empty values and deduplicates"), Normalized.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuntimeAssetPreloadCompletionTest,
                                 "ReEcho.Presentation.RuntimeAssetPreload.Completion",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuntimeAssetPreloadCompletionTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UReEchoRuntimeAssetPreloader* EmptyPreloader = NewObject<UReEchoRuntimeAssetPreloader>(GameInstance);
	EmptyPreloader->BeginPreloadForTests({});
	int32 EmptyCallbackCount = 0;
	EmptyPreloader->RequestPreload(FSimpleDelegate::CreateLambda(
	    [&EmptyCallbackCount]()
	    {
		    ++EmptyCallbackCount;
	    }));
	EmptyPreloader->CompletePreloadForTests(true);
	TestTrue(TEXT("An empty list completes successfully"), EmptyPreloader->DidPreloadSucceed());
	TestEqual(TEXT("Completion is idempotent and invokes the callback once"), EmptyCallbackCount, 1);

	UReEchoRuntimeAssetPreloader* FailedPreloader = NewObject<UReEchoRuntimeAssetPreloader>(GameInstance);
	FailedPreloader->CompletePreloadForTests(false);
	int32 FailureCallbackCount = 0;
	FailedPreloader->RequestPreload(FSimpleDelegate::CreateLambda(
	    [&FailureCallbackCount]()
	    {
		    ++FailureCallbackCount;
	    }));
	FailedPreloader->CompletePreloadForTests(false);
	TestTrue(TEXT("A failed request still reaches a terminal state"), FailedPreloader->IsPreloadComplete());
	TestFalse(TEXT("Failure remains observable"), FailedPreloader->DidPreloadSucceed());
	TestEqual(TEXT("Failure releases waiting callers exactly once"), FailureCallbackCount, 1);

	GameInstance->MarkAsGarbage();
	return true;
}

#endif
