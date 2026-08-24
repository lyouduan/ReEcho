#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Cards/ReEchoCardTypes.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/ReEchoCsvDataRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Graybox/ReEchoTimeShardPickupActor.h"
#include "Run/ReEchoRunSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyShardDropDataTest,
                                 "ReEcho.Run.EnemyShardDrops.DataContract",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyShardDropDataTest::RunTest(const FString& Parameters)
{
	const TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot = FReEchoCsvDataRegistry::GetSnapshot();
	if (!TestTrue(TEXT("Production data snapshot is available"), Snapshot.IsValid()))
	{
		return false;
	}
	TestEqual(TEXT("All eight encounter rows are present"), Snapshot->EnemyShardDrops.Num(), 8);
	const int32 ExpectedMeleeMin[] = {2, 2, 2, 2, 2, 2, 2, 2};
	const int32 ExpectedMeleeMax[] = {3, 3, 3, 3, 3, 3, 3, 3};
	const int32 ExpectedRangedMin[] = {4, 4, 4, 4, 7, 7, 7, 7};
	const int32 ExpectedRangedMax[] = {6, 6, 6, 6, 9, 9, 9, 9};
	const int32 ExpectedEliteMin[] = {INDEX_NONE, INDEX_NONE, 10, 10, 15, 15, 15, 15};
	const int32 ExpectedEliteMax[] = {INDEX_NONE, INDEX_NONE, 13, 13, 20, 20, 20, 20};
	for (int32 EncounterIndex = 1; EncounterIndex <= 8; ++EncounterIndex)
	{
		const FReEchoCsvEnemyShardDropRow* Row = Snapshot->FindEnemyShardDrop(EncounterIndex);
		if (!TestNotNull(*FString::Printf(TEXT("Encounter %d row resolves"), EncounterIndex), Row))
		{
			continue;
		}
		const int32 ArrayIndex = EncounterIndex - 1;
		TestEqual(TEXT("Melee minimum matches source"), Row->MeleeMin, ExpectedMeleeMin[ArrayIndex]);
		TestEqual(TEXT("Melee maximum matches source"), Row->MeleeMax, ExpectedMeleeMax[ArrayIndex]);
		TestEqual(TEXT("Ranged minimum matches source"), Row->RangedMin, ExpectedRangedMin[ArrayIndex]);
		TestEqual(TEXT("Ranged maximum matches source"), Row->RangedMax, ExpectedRangedMax[ArrayIndex]);
		TestEqual(TEXT("Elite minimum matches source"), Row->EliteMin, ExpectedEliteMin[ArrayIndex]);
		TestEqual(TEXT("Elite maximum matches source"), Row->EliteMax, ExpectedEliteMax[ArrayIndex]);
	}
	const FReEchoCsvEnemyShardDropRow* First = Snapshot->FindEnemyShardDrop(1);
	const FReEchoCsvEnemyShardDropRow* Third = Snapshot->FindEnemyShardDrop(3);
	const FReEchoCsvEnemyShardDropRow* Fifth = Snapshot->FindEnemyShardDrop(5);
	if (!TestNotNull(TEXT("Encounter one row resolves"), First) ||
	    !TestNotNull(TEXT("Encounter three row resolves"), Third) ||
	    !TestNotNull(TEXT("Encounter five row resolves"), Fifth))
	{
		return false;
	}
	TestEqual(TEXT("Encounter one melee minimum"), First->MeleeMin, 2);
	TestEqual(TEXT("Encounter one ranged maximum"), First->RangedMax, 6);
	TestEqual(TEXT("Encounter one has no elite reward"), First->EliteMin, INDEX_NONE);
	TestEqual(TEXT("Encounter three elite range begins at ten"), Third->EliteMin, 10);
	TestEqual(TEXT("Encounter five ranged range begins at seven"), Fifth->RangedMin, 7);
	TestEqual(TEXT("Encounter five elite range ends at twenty"), Fifth->EliteMax, 20);
	TestNotNull(TEXT("Reviewed pickup texture is available to the runtime"),
	            LoadObject<UTexture2D>(nullptr, TEXT("/Game/ReEcho/Textures/Pickups/T_TimeShard.T_TimeShard")));
	UClass* PickupBlueprintClass = LoadClass<AReEchoTimeShardPickupActor>(
	    nullptr,
	    TEXT("/Game/ReEcho/Gameplay/Pickups/BP_TimeShardPickup.BP_TimeShardPickup_C"));
	TestNotNull(TEXT("Editor-authored pickup Blueprint is available"), PickupBlueprintClass);
	const AReEchoTimeShardPickupActor* PickupDefaults =
	    PickupBlueprintClass ? Cast<AReEchoTimeShardPickupActor>(PickupBlueprintClass->GetDefaultObject()) : nullptr;
	const UMaterialBillboardComponent* PickupVisual = PickupDefaults ? PickupDefaults->GetVisualComponent() : nullptr;
	TestNotNull(TEXT("Pickup uses a translucent-capable material billboard"), PickupVisual);
	if (PickupVisual)
	{
		TestTrue(TEXT("Pickup Blueprint supplies a material billboard element"), !PickupVisual->Elements.IsEmpty());
	}
	const UStaticMeshComponent* PickupShadow = PickupDefaults ? PickupDefaults->GetGroundShadowComponent() : nullptr;
	TestNotNull(TEXT("Pickup Blueprint exposes a ground shadow component"), PickupShadow);
	TestTrue(TEXT("Pickup Blueprint keeps a positive editor-authored visual height"),
	         PickupDefaults && PickupDefaults->GetVisualWorldHeightCm() > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoEnemyShardDropRuntimeTest,
                                 "ReEcho.Run.EnemyShardDrops.Runtime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoEnemyShardDropRuntimeTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UReEchoRunSubsystem* Run = NewObject<UReEchoRunSubsystem>(GameInstance);
	Run->StartRun(TEXT("J_SPADE"), TEXT("W_J_02"));
	Run->BeginEncounter();
	const int32 InitialBalance = Run->TimeShards;

	const int32 MeleeReward = Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), 1);
	TestTrue(TEXT("Melee reward uses encounter-one 2..3 range"), MeleeReward >= 2 && MeleeReward <= 3);
	TestEqual(
	    TEXT("Duplicate death notification grants nothing"), Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), 1), 0);
	const int32 RangedReward = Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_RABBIT"), 2);
	TestTrue(TEXT("Ranged reward uses encounter-one 4..6 range"), RangedReward >= 4 && RangedReward <= 6);
	TestEqual(TEXT("Encounter one has no elite reward"), Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_FOX"), 3), 0);
	TestEqual(
	    TEXT("Bosses never use the regular enemy reward table"), Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_SHEEP"), 4), 0);

	Run->CurrentBuild.CardState.Runtime.BonusShardDropEncounterIndex = Run->EncounterIndex;
	const int32 BonusReward = Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), 5);
	TestTrue(TEXT("One-shot bonus multiplies and rounds each enemy reward"), BonusReward >= 3 && BonusReward <= 5);
	Run->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::NoEnemyShardDrops;
	TestEqual(
	    TEXT("No-drop penalty suppresses the enemy reward"), Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), 6), 0);
	Run->CurrentBuild.CardState.Runtime.EconomyPenalty = EReEchoCardEconomyPenalty::None;
	TestEqual(TEXT("Suppressed deaths remain idempotent after the penalty changes"),
	          Run->ResolveEnemyDeathTimeShardDrop(TEXT("M_Grunt"), 6),
	          0);
	TestEqual(TEXT("Resolving enemy deaths never changes the balance before collection"),
	          Run->TimeShards,
	          InitialBalance);
	return true;
}

#endif
