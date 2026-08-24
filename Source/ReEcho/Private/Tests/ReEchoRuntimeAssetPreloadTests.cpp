#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Presentation/Loading/ReEchoRuntimeAssetPreloader.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoRuntimeAssetPreloadCatalogTest,
                                 "ReEcho.Presentation.RuntimeAssetPreload.Catalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoRuntimeAssetPreloadCatalogTest::RunTest(const FString& Parameters)
{
	const TArray<FSoftObjectPath> AssetPaths = UReEchoRuntimeAssetPreloader::BuildDefaultAssetList();
	TArray<FString> GatheredPaths;
	FReEchoCombatVfxCatalog::GatherPreloadAssetPaths(GatheredPaths);
	FReEchoElementReactionVfxCatalog::GatherPreloadAssetPaths(GatheredPaths);
	FReEchoWeaponVisualCatalog::GatherPreloadAssetPaths(GatheredPaths);
	const TArray<FSoftObjectPath> ExpectedPaths = UReEchoRuntimeAssetPreloader::NormalizeAssetPaths(GatheredPaths);
	TestEqual(
	    TEXT("Default preload count matches the three authoritative catalogs"), AssetPaths.Num(), ExpectedPaths.Num());
	for (int32 PathIndex = 0; PathIndex < FMath::Min(AssetPaths.Num(), ExpectedPaths.Num()); ++PathIndex)
	{
		TestTrue(FString::Printf(TEXT("Default preload path %d preserves authoritative catalog order"), PathIndex),
		         AssetPaths[PathIndex] == ExpectedPaths[PathIndex]);
	}
	TestFalse(TEXT("The authoritative preload list is not empty"), AssetPaths.IsEmpty());
	TSet<FSoftObjectPath> UniquePaths;
	for (const FSoftObjectPath& AssetPath : AssetPaths)
	{
		TestTrue(TEXT("Every preload path is valid"), AssetPath.IsValid());
		TestFalse(TEXT("Every preload path is unique"), UniquePaths.Contains(AssetPath));
		UniquePaths.Add(AssetPath);
	}
	TestTrue(TEXT("Longsword Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Sword/Particle/NS_People_Sword_Attack_01.NS_People_Sword_Attack_01"))));
	TestTrue(TEXT("Scythe Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Sickle/Particle/NS_People_Sickle_Attack_01.NS_People_Sickle_Attack_01"))));
	TestTrue(TEXT("Bow flight Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Bow/Particle/NS_People_Bow_Attack_01.NS_People_Bow_Attack_01"))));
	TestTrue(TEXT("Gun impact Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/People/Bullet/Particle/NS_People_Bullet_spark.NS_People_Bullet_spark"))));
	TestFalse(TEXT("Missing Whip specialist art is not preloaded or borrowed"),
	          UniquePaths.Contains(FSoftObjectPath(TEXT("/Game/ReEcho/Textures/Effects/WhipLash.WhipLash"))));
	TestTrue(TEXT("Grass attachment Niagara is preloaded"),
	         UniquePaths.Contains(
	             FSoftObjectPath(TEXT("/Game/VFX/Element/Grass/Particle/NS_Element_Grass.NS_Element_Grass"))));
	TestTrue(TEXT("Conduct Niagara is preloaded"),
	         UniquePaths.Contains(FSoftObjectPath(
	             TEXT("/Game/VFX/Element/Elctricity/Particle/NS_Element_Electricity.NS_Element_Electricity"))));

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
