#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Combat/ReEchoElementReaction.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Presentation/Animation2D/ReEcho2DCharacterPresentationProfile.h"
#include "UI/ReEchoDamageNumberActor.h"
#include "UI/ReEchoElementReactionPopupActor.h"
#include "UI/ReEchoEncounterHudWidget.h"
#include "UI/ReEchoBossHealthArcWidget.h"
#include "UI/ReEchoMinimapCanvasWidget.h"
#include "UI/ReEchoPlayerHudWidget.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "MaterialShared.h"
#include "ShaderCompilerCore.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/Layout/SBorder.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoDamageNumberPoolingTest,
                                 "ReEcho.UI.CombatHud.DamageNumberPooling",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoDamageNumberPoolingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	if (!TestNotNull(TEXT("Damage-number pool test creates a world"), World))
	{
		return false;
	}
	World->AddToRoot();
	AReEchoDamageNumberActor::SpawnDamageNumber(World, FVector::ZeroVector, 10.0f, FLinearColor::White);
	TArray<AReEchoDamageNumberActor*> DamageNumbers;
	for (TObjectIterator<AReEchoDamageNumberActor> It; It; ++It)
	{
		if (It->GetWorld() == World)
		{
			DamageNumbers.Add(*It);
		}
	}
	TestEqual(TEXT("First damage event creates one actor"), DamageNumbers.Num(), 1);
	if (DamageNumbers.Num() == 1)
	{
		DamageNumbers[0]->Tick(2.0f);
	}
	TestEqual(TEXT("Expired damage number returns to its world pool"),
	          AReEchoDamageNumberActor::GetPooledCountForTests(World),
	          1);
	AReEchoDamageNumberActor::SpawnDamageNumber(World, FVector(100.0f, 0.0f, 0.0f), 20.0f, FLinearColor::Red);
	TestEqual(TEXT("Next damage event reuses the pooled actor"),
	          AReEchoDamageNumberActor::GetPooledCountForTests(World),
	          0);
	int32 ReusedActorCount = 0;
	for (TObjectIterator<AReEchoDamageNumberActor> It; It; ++It)
	{
		ReusedActorCount += It->GetWorld() == World ? 1 : 0;
	}
	TestEqual(TEXT("Pooling avoids creating a second actor"), ReusedActorCount, 1);
	World->DestroyWorld(true);
	World->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoCombatHudFormattingTest,
                                 "ReEcho.UI.CombatHud.Formatting",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FReEchoCombatHudFormattingTest::RunTest(const FString& Parameters)
{
	UTexture2D* MinimapInkTip =
	    LoadObject<UTexture2D>(nullptr, UReEchoMinimapCanvasWidget::GetInkBrushTipTexturePath());
	UTexture2D* MinimapInkGrain = LoadObject<UTexture2D>(nullptr, UReEchoMinimapCanvasWidget::GetInkGrainTexturePath());
	UMaterialInterface* MinimapInkMaterial =
	    LoadObject<UMaterialInterface>(nullptr, UReEchoMinimapCanvasWidget::GetInkTrailMaterialPath());
	TestNotNull(TEXT("Minimap ink brush-tip texture loads"), MinimapInkTip);
	TestNotNull(TEXT("Minimap ink grain texture loads"), MinimapInkGrain);
	TestNotNull(TEXT("Minimap ink UI material loads"), MinimapInkMaterial);
	if (MinimapInkMaterial)
	{
		TestEqual(TEXT("Minimap ink material is translucent"),
		          MinimapInkMaterial->GetBlendMode(),
		          EBlendMode::BLEND_Translucent);
		TestEqual(TEXT("Minimap ink material uses the UI domain"),
		          static_cast<EMaterialDomain>(MinimapInkMaterial->GetMaterial()->MaterialDomain),
		          EMaterialDomain::MD_UI);
	}

	const TPair<FName, FString> ReactionPopupTextures[] = {
	    {TEXT("Reaction.Burn"), TEXT("T_UI_Reaction_Burn")},
	    {TEXT("Reaction.Vaporize"), TEXT("T_UI_Reaction_Vaporize")},
	    {TEXT("Reaction.Growth"), TEXT("T_UI_Reaction_Growth")},
	    {TEXT("Reaction.Conduct"), TEXT("T_UI_Reaction_Conduct")},
	    {TEXT("Reaction.Enhance"), TEXT("T_UI_Reaction_Enhance")},
	};
	for (const TPair<FName, FString>& ReactionPopup : ReactionPopupTextures)
	{
		const TCHAR* TexturePath = AReEchoElementReactionPopupActor::GetReactionTexturePath(ReactionPopup.Key);
		const FString MappingWhat = FString::Printf(TEXT("Reaction popup maps %s"), *ReactionPopup.Key.ToString());
		TestNotNull(*MappingWhat, TexturePath);
		if (TexturePath)
		{
			const FString PathWhat = FString::Printf(TEXT("Reaction popup path names %s"), *ReactionPopup.Value);
			TestTrue(*PathWhat, FString(TexturePath).Contains(ReactionPopup.Value));
			const FString LoadWhat = FString::Printf(TEXT("Reaction popup texture loads: %s"), TexturePath);
			TestNotNull(*LoadWhat, LoadObject<UTexture2D>(nullptr, TexturePath));
		}
	}
	TestNull(TEXT("Unknown reaction does not invent a popup asset"),
	         AReEchoElementReactionPopupActor::GetReactionTexturePath(TEXT("Reaction.Unknown")));
	TestEqual(TEXT("Reaction popup starts opaque"),
	          AReEchoElementReactionPopupActor::CalculateOpacity(0.0f, 0.15f, 1.3f),
	          1.0f);
	TestTrue(TEXT("Reaction popup fades during its lifetime"),
	         AReEchoElementReactionPopupActor::CalculateOpacity(0.6f, 0.15f, 1.3f) > 0.0f &&
	             AReEchoElementReactionPopupActor::CalculateOpacity(0.6f, 0.15f, 1.3f) < 1.0f);
	TestEqual(TEXT("Reaction popup ends transparent"),
	          AReEchoElementReactionPopupActor::CalculateOpacity(1.0f, 0.15f, 1.3f),
	          0.0f);

	UMaterialInterface* ReactionPopupMaterial =
	    LoadObject<UMaterialInterface>(nullptr, AReEchoElementReactionPopupActor::GetPopupMaterialPath());
	TestNotNull(TEXT("Reaction-popup translucent material loads"), ReactionPopupMaterial);
	if (ReactionPopupMaterial)
	{
		TestEqual(TEXT("Reaction-popup material is translucent"),
		          ReactionPopupMaterial->GetBlendMode(),
		          EBlendMode::BLEND_Translucent);
	}
	UClass* ReactionPopupBlueprintClass = LoadClass<AReEchoElementReactionPopupActor>(
	    nullptr, AReEchoElementReactionPopupActor::GetPopupBlueprintClassPath());
	TestNotNull(TEXT("Reaction-popup art settings Blueprint loads"), ReactionPopupBlueprintClass);
	if (ReactionPopupBlueprintClass)
	{
		TestTrue(TEXT("Reaction-popup Blueprint derives from the native actor"),
		         ReactionPopupBlueprintClass->IsChildOf(AReEchoElementReactionPopupActor::StaticClass()));
	}

	UMaterialInterface* DamageNumberMaterial =
	    LoadObject<UMaterialInterface>(nullptr, AReEchoDamageNumberActor::GetDamageNumberMaterialPath());
	TestNotNull(TEXT("Damage-number translucent material loads"), DamageNumberMaterial);
	if (DamageNumberMaterial)
	{
		TestEqual(TEXT("Damage-number material supports translucent distance-field glyphs"),
		          DamageNumberMaterial->GetBlendMode(),
		          EBlendMode::BLEND_Translucent);
	}

	auto TestReactionDamageColor = [this](const TCHAR* What, const FName ReactionBehaviorId, const FColor Expected)
	{
		FReEchoDamageEvent Event;
		Event.ReactionBehaviorId = ReactionBehaviorId;
		TestEqual(What, ReEchoElementReaction::GetDamageNumberColor(Event).ToFColor(true), Expected);
	};
	TestReactionDamageColor(
	    TEXT("Vaporize damage numbers use reference light blue"), TEXT("Reaction.Vaporize"), FColor(165, 203, 243));
	TestReactionDamageColor(
	    TEXT("Conduct damage numbers use reference yellow"), TEXT("Reaction.Conduct"), FColor(235, 192, 44));
	TestReactionDamageColor(
	    TEXT("Burn damage numbers use reference orange"), TEXT("Reaction.Burn"), FColor(232, 106, 18));
	TestReactionDamageColor(
	    TEXT("Growth damage numbers use reference green"), TEXT("Reaction.Growth"), FColor(146, 192, 57));
	TestReactionDamageColor(
	    TEXT("Enhanced damage numbers use reference gold"), TEXT("Reaction.Enhance"), FColor(241, 184, 76));

	FReEchoDamageEvent OrdinaryDamageEvent;
	OrdinaryDamageEvent.RawDamage = 7.0f;
	OrdinaryDamageEvent.AppliedDamage = 7.0f;
	TestEqual(TEXT("Ordinary damage numbers preserve the resolved damage"),
	          ReEchoElementReaction::GetDamageNumberValue(OrdinaryDamageEvent),
	          7.0f);
	FReEchoDamageEvent OverkillDamageEvent;
	OverkillDamageEvent.RawDamage = 20.0f;
	OverkillDamageEvent.AppliedDamage = 7.0f;
	TestEqual(TEXT("Overkill damage numbers show pre-health-clamp damage"),
	          ReEchoElementReaction::GetDamageNumberValue(OverkillDamageEvent),
	          20.0f);

	UFont* DamageNumberFont = LoadObject<UFont>(nullptr, AReEchoDamageNumberActor::GetDamageNumberFontPath());
	TestNotNull(TEXT("Damage-number actor owns the approved non-commercial runtime font"), DamageNumberFont);
	if (DamageNumberFont)
	{
		TestEqual(TEXT("Damage-number font uses TextRender-compatible offline caching"),
		          DamageNumberFont->FontCacheType,
		          EFontCacheType::Offline);
		TestTrue(TEXT("Damage-number font contains a baked glyph texture"), !DamageNumberFont->Textures.IsEmpty());
	}
	UClass* DamageNumberBlueprintClass =
	    LoadClass<AReEchoDamageNumberActor>(nullptr, AReEchoDamageNumberActor::GetDamageNumberBlueprintClassPath());
	TestNotNull(TEXT("Damage-number animation settings Blueprint loads"), DamageNumberBlueprintClass);
	if (DamageNumberBlueprintClass)
	{
		TestTrue(TEXT("Damage-number Blueprint derives from the native actor"),
		         DamageNumberBlueprintClass->IsChildOf(AReEchoDamageNumberActor::StaticClass()));
		const FFloatProperty* CriticalSizeScaleProperty =
		    FindFProperty<FFloatProperty>(DamageNumberBlueprintClass, TEXT("CriticalSizeScale"));
		TestNotNull(TEXT("Damage-number Blueprint exposes an independent critical size scale"),
		            CriticalSizeScaleProperty);
		if (CriticalSizeScaleProperty)
		{
			const float CriticalSizeScale =
			    CriticalSizeScaleProperty->GetPropertyValue_InContainer(DamageNumberBlueprintClass->GetDefaultObject());
			TestTrue(TEXT("Damage-number critical size scale is positive"), CriticalSizeScale > 0.0f);
		}
	}

	TestEqual(TEXT("Encounter label follows the visual spec"),
	          UReEchoEncounterHudWidget::FormatEncounterLabel(3).ToString(),
	          FString(TEXT("第 3 关")));
	TestEqual(TEXT("Countdown rounds up partial seconds"),
	          UReEchoEncounterHudWidget::FormatCountdown(59.1f).ToString(),
	          FString(TEXT("01:00")));
	TestEqual(TEXT("Countdown clamps negative values"),
	          UReEchoEncounterHudWidget::FormatCountdown(-1.0f).ToString(),
	          FString(TEXT("00:00")));
	TestEqual(TEXT("Countdown needle starts on the left"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(60.0f, 60.0f),
	          90.0f);
	TestEqual(TEXT("Countdown needle points down at half time"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(30.0f, 60.0f),
	          0.0f);
	TestEqual(TEXT("Countdown needle finishes on the right"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(0.0f, 60.0f),
	          -90.0f);
	TestEqual(TEXT("Countdown needle clamps remaining time above duration"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(90.0f, 60.0f),
	          90.0f);
	TestEqual(TEXT("Countdown needle safely handles an invalid duration before completion"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(10.0f, 0.0f),
	          90.0f);
	TestEqual(TEXT("Countdown needle safely handles an invalid duration at completion"),
	          UReEchoEncounterHudWidget::CalculateCountdownNeedleAngle(0.0f, 0.0f),
	          -90.0f);
	TestEqual(TEXT("Boss health ratio uses authoritative current and maximum health"),
	          UReEchoEncounterHudWidget::CalculateBossHealthRatio(650.0f, 1300.0f),
	          0.5f);
	TestEqual(TEXT("Boss health ratio clamps over-heal"),
	          UReEchoEncounterHudWidget::CalculateBossHealthRatio(1400.0f, 1300.0f),
	          1.0f);
	TestEqual(TEXT("Boss health ratio safely handles an invalid maximum"),
	          UReEchoEncounterHudWidget::CalculateBossHealthRatio(650.0f, 0.0f),
	          0.0f);
	TestEqual(TEXT("Boss needle clamps negative ratios"),
	          UReEchoEncounterHudWidget::CalculateBossHealthNeedleAngle(-1.0f),
	          -90.0f);
	TestEqual(TEXT("Boss needle clamps over-heal ratios"),
	          UReEchoEncounterHudWidget::CalculateBossHealthNeedleAngle(2.0f),
	          90.0f);
	TestEqual(TEXT("Half Boss health reads fifty percent"),
	          UReEchoEncounterHudWidget::FormatBossHealthPercent(0.5f).ToString(),
	          FString(TEXT("50%")));
	TestEqual(TEXT("Live Boss never reads zero percent"),
	          UReEchoEncounterHudWidget::FormatBossHealthPercent(0.001f).ToString(),
	          FString(TEXT("1%")));
	TestEqual(TEXT("Wounded Boss never reads full percent"),
	          UReEchoEncounterHudWidget::FormatBossHealthPercent(0.999f).ToString(),
	          FString(TEXT("99%")));
	TestEqual(TEXT("Negative ratio reads zero percent"),
	          UReEchoEncounterHudWidget::FormatBossHealthPercent(-1.0f).ToString(),
	          FString(TEXT("0%")));
	TestEqual(TEXT("Over-heal reads full percent"),
	          UReEchoEncounterHudWidget::FormatBossHealthPercent(2.0f).ToString(),
	          FString(TEXT("100%")));

	UReEchoPlayerHudWidget* PlayerHud = NewObject<UReEchoPlayerHudWidget>();
	PlayerHud->SetTimeShards(12);
	TestEqual(TEXT("Time shards retain the live run value"), PlayerHud->GetTimeShardsForTests(), 12);
	PlayerHud->SetTimeShards(-5);
	TestEqual(TEXT("Time-shard HUD preserves the authoritative signed net balance for shop debt"),
	          PlayerHud->GetTimeShardsForTests(),
	          -5);

	UClass* PlayerHudClass =
	    LoadClass<UReEchoPlayerHudWidget>(nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoPlayerHud.WBP_ReEchoPlayerHud_C"));
	TestNotNull(TEXT("Authored Player HUD class loads"), PlayerHudClass);
	if (!PlayerHudClass)
	{
		return false;
	}
	UReEchoPlayerHudWidget* AuthoredPlayerHud =
	    NewObject<UReEchoPlayerHudWidget>(GetTransientPackage(), PlayerHudClass);
	TestTrue(TEXT("Authored Player HUD initializes"), AuthoredPlayerHud->Initialize());
	AuthoredPlayerHud->TakeWidget();
	AuthoredPlayerHud->SetTimeShards(27);
	UTextBlock* TimeShardText = Cast<UTextBlock>(AuthoredPlayerHud->GetWidgetFromName(TEXT("TimeShardText")));
	TestNotNull(TEXT("Player HUD exposes the time-shard binding"), TimeShardText);
	if (TimeShardText)
	{
		TestEqual(TEXT("Time-shard binding renders the live balance"),
		          TimeShardText->GetText().ToString(),
		          FString(TEXT("27")));
		AuthoredPlayerHud->SetTimeShards(22);
		TestEqual(TEXT("Time-shard binding refreshes while an overlay pauses normal gameplay ticks"),
		          TimeShardText->GetText().ToString(),
		          FString(TEXT("22")));
	}
	TestNotNull(TEXT("Player HUD exposes the image health fill"),
	            Cast<UImage>(AuthoredPlayerHud->GetWidgetFromName(TEXT("PlayerHealthFill"))));
	UImage* PlayerHealthFrame = Cast<UImage>(AuthoredPlayerHud->GetWidgetFromName(TEXT("ArtHealthFrame")));
	TestNotNull(TEXT("Player HUD exposes the authored health frame"), PlayerHealthFrame);
	USizeBox* PortraitSize = Cast<USizeBox>(AuthoredPlayerHud->GetWidgetFromName(TEXT("PlayerPortraitSize")));
	TestNotNull(TEXT("Legacy portrait host is retained for compatibility"), PortraitSize);
	if (PortraitSize)
	{
		TestEqual(TEXT("Legacy portrait is absent from the authored composition"),
		          PortraitSize->GetVisibility(),
		          ESlateVisibility::Collapsed);
	}

	UClass* EncounterHudClass = LoadClass<UReEchoEncounterHudWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoEncounterHud.WBP_ReEchoEncounterHud_C"));
	TestNotNull(TEXT("Authored Encounter HUD class loads"), EncounterHudClass);
	if (!EncounterHudClass)
	{
		return false;
	}
	UReEchoEncounterHudWidget* AuthoredEncounterHud =
	    NewObject<UReEchoEncounterHudWidget>(GetTransientPackage(), EncounterHudClass);
	TestTrue(TEXT("Authored Encounter HUD initializes"), AuthoredEncounterHud->Initialize());
	AuthoredEncounterHud->TakeWidget();
	AuthoredEncounterHud->SetEncounterStatus(3, 6, 59.1f, 60.0f);
	UTextBlock* EncounterText = Cast<UTextBlock>(AuthoredEncounterHud->GetWidgetFromName(TEXT("EncounterText")));
	UTextBlock* CountdownText = Cast<UTextBlock>(AuthoredEncounterHud->GetWidgetFromName(TEXT("CountdownText")));
	UTextBlock* BossPercentText =
	    Cast<UTextBlock>(AuthoredEncounterHud->GetWidgetFromName(TEXT("BossHealthPercentText")));
	UImage* ClockNeedle = Cast<UImage>(AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtClockNeedle")));
	UImage* ClockFrame = Cast<UImage>(AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtClockFrame")));
	UReEchoBossHealthArcWidget* BossHealthArc =
	    Cast<UReEchoBossHealthArcWidget>(AuthoredEncounterHud->GetWidgetFromName(TEXT("BossHealthArc")));
	TestNotNull(TEXT("Encounter HUD keeps the encounter label binding"), EncounterText);
	TestNotNull(TEXT("Encounter HUD keeps the countdown binding"), CountdownText);
	TestNotNull(TEXT("Boss percentage is an authored, separately editable text block"), BossPercentText);
	TestNotNull(TEXT("Encounter HUD keeps the animated clock needle binding"), ClockNeedle);
	TestNotNull(TEXT("Encounter HUD keeps the time frame binding"), ClockFrame);
	TestNotNull(TEXT("Encounter HUD exposes the authored health arc"), BossHealthArc);
	TestNull(TEXT("Replaced horizontal Boss panel is removed"),
	         AuthoredEncounterHud->GetWidgetFromName(TEXT("BossHealthPanel")));
	TestNull(TEXT("Replaced horizontal Boss frame is removed"),
	         AuthoredEncounterHud->GetWidgetFromName(TEXT("BossHealthFrame")));
	if (BossHealthArc)
	{
		TestNotNull(TEXT("Arc has a cooked hard reference to its UI material"), BossHealthArc->ArcMaterial.Get());
		// Color/opacity are artist-owned, not a contract for one initial palette.
		TestTrue(TEXT("Authored tick contrast stays in the editable range"),
		         BossHealthArc->TickContrast >= 0.0f && BossHealthArc->TickContrast <= 1.0f);
	}
	TestNull(TEXT("Rejected countdown background is absent"),
	         AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtTimeReadout")));
	if (EncounterText && CountdownText)
	{
		TestEqual(
		    TEXT("Authored encounter label refreshes"), EncounterText->GetText().ToString(), FString(TEXT("第 3 关")));
		TestEqual(TEXT("Authored countdown refreshes"), CountdownText->GetText().ToString(), FString(TEXT("01:00")));
	}
	if (ClockFrame && ClockNeedle && CountdownText && BossHealthArc && BossPercentText)
	{
		const FSlateFontInfo PercentFont = BossPercentText->GetFont();
		const FSlateColor PercentColor = BossPercentText->GetColorAndOpacity();
		UCanvasPanelSlot* PercentSlot = Cast<UCanvasPanelSlot>(BossPercentText->Slot);
		const FVector2D PercentPosition = PercentSlot ? PercentSlot->GetPosition() : FVector2D::ZeroVector;
		TestEqual(TEXT("Normal encounter hides Boss percentage"),
		          BossPercentText->GetVisibility(),
		          ESlateVisibility::Collapsed);
		TestEqual(TEXT("Normal encounter shows the time frame"),
		          ClockFrame->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Normal encounter shows the countdown"),
		          CountdownText->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Normal encounter shows the clock needle"),
		          ClockNeedle->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Normal encounter hides the Boss health arc"),
		          BossHealthArc->GetVisibility(),
		          ESlateVisibility::Collapsed);

		AuthoredEncounterHud->SetEncounterStatus(8, 8, 40.0f, 60.0f, true, 650.0f, 1300.0f);
		TestEqual(TEXT("Boss encounter keeps the decorative clock frame"),
		          ClockFrame->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(
		    TEXT("Boss encounter hides the countdown"), CountdownText->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("Boss encounter retains the clock needle"),
		          ClockNeedle->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Boss encounter shows the Boss health arc"),
		          BossHealthArc->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Boss arc follows the authoritative ratio"), BossHealthArc->GetHealthRatio(), 0.5f);
		TestEqual(TEXT("Boss half health needle points down"), ClockNeedle->GetRenderTransform().Angle, 0.0f);
		TestEqual(
		    TEXT("Boss percentage is visible"), BossPercentText->GetVisibility(), ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Boss percentage uses health, not remaining time"),
		          BossPercentText->GetText().ToString(),
		          FString(TEXT("50%")));
		const FWidgetTransform AuthoredTransform = BossHealthArc->GetRenderTransform();
		UCanvasPanelSlot* ArcSlot = Cast<UCanvasPanelSlot>(BossHealthArc->Slot);
		TestNotNull(TEXT("Arc is directly editable in the HUD Canvas"), ArcSlot);
		const FVector2D AuthoredPosition = ArcSlot ? ArcSlot->GetPosition() : FVector2D::ZeroVector;
		const FVector2D AuthoredSize = ArcSlot ? ArcSlot->GetSize() : FVector2D::ZeroVector;
		BossHealthArc->FillColor = FLinearColor(0.1f, 0.7f, 0.3f, 0.85f);
		BossHealthArc->TickContrast = 0.6f;
		BossHealthArc->VeinStrength = 0.7f;
		BossHealthArc->VeinWidth = 2.2f;
		BossHealthArc->VeinSpacing = 63.0f;
		BossHealthArc->ReliefStrength = 0.8f;
		BossHealthArc->SynchronizeProperties();
		AuthoredEncounterHud->SetEncounterStatus(8, 8, 1.0f, 60.0f, true, 0.0f, 1300.0f);
		TestEqual(TEXT("Empty arc has no remaining fill"), BossHealthArc->GetHealthRatio(), 0.0f);
		TestEqual(
		    TEXT("Empty Boss health reads zero percent"), BossPercentText->GetText().ToString(), FString(TEXT("0%")));
		TestEqual(TEXT("Empty needle points right independent of countdown"),
		          ClockNeedle->GetRenderTransform().Angle,
		          -90.0f);
		AuthoredEncounterHud->SetEncounterStatus(8, 8, 0.0f, 60.0f, true, 2000.0f, 2000.0f);
		TestEqual(TEXT("New phase health refill restores the full arc"), BossHealthArc->GetHealthRatio(), 1.0f);
		TestEqual(TEXT("Full needle returns left"), ClockNeedle->GetRenderTransform().Angle, 90.0f);
		TestEqual(TEXT("Phase refill restores one hundred percent"),
		          BossPercentText->GetText().ToString(),
		          FString(TEXT("100%")));
		TestEqual(TEXT("Status updates preserve authored tick contrast"), BossHealthArc->TickContrast, 0.6f);
		const FObjectProperty* MaterialProperty =
		    FindFProperty<FObjectProperty>(BossHealthArc->GetClass(), TEXT("DynamicMaterial"));
		UMaterialInstanceDynamic* SurfaceMaterial =
		    MaterialProperty
		        ? Cast<UMaterialInstanceDynamic>(MaterialProperty->GetObjectPropertyValue_InContainer(BossHealthArc))
		        : nullptr;
		if (TestNotNull(TEXT("Arc owns a live material for surface controls"), SurfaceMaterial))
		{
			for (const TPair<FName, float>& Control : {TPair<FName, float>(TEXT("VeinStrength"), 0.7f),
			                                           {TEXT("VeinWidth"), 2.2f},
			                                           {TEXT("VeinSpacing"), 63.0f},
			                                           {TEXT("ReliefStrength"), 0.8f}})
			{
				float Value = -1.0f;
				TestTrue(TEXT("Live material exposes surface parameter"),
				         SurfaceMaterial->GetScalarParameterValue(Control.Key, Value));
				TestEqual(*FString::Printf(TEXT("Health updates preserve %s"), *Control.Key.ToString()),
				          Value,
				          Control.Value);
			}
		}
		TestTrue(TEXT("Status updates preserve percentage font"), BossPercentText->GetFont() == PercentFont);
		TestTrue(TEXT("Status updates preserve percentage color"),
		         BossPercentText->GetColorAndOpacity() == PercentColor);
		if (PercentSlot)
		{
			TestEqual(TEXT("Status updates preserve percentage position"), PercentSlot->GetPosition(), PercentPosition);
		}
		TestTrue(TEXT("Status updates preserve authored color"),
		         BossHealthArc->FillColor.Equals(FLinearColor(0.1f, 0.7f, 0.3f, 0.85f)));
		TestTrue(TEXT("Status updates do not scale the arc geometry"),
		         BossHealthArc->GetRenderTransform() == AuthoredTransform);
		if (ArcSlot)
		{
			TestEqual(TEXT("Status updates preserve authored position"), ArcSlot->GetPosition(), AuthoredPosition);
			TestEqual(TEXT("Status updates preserve authored size"), ArcSlot->GetSize(), AuthoredSize);
		}
		AuthoredEncounterHud->SetEncounterStatus(1, 8, 15.0f, 30.0f);
		TestEqual(TEXT("Switching back hides the arc"), BossHealthArc->GetVisibility(), ESlateVisibility::Collapsed);
		TestEqual(TEXT("Switching back hides the Boss percentage"),
		          BossPercentText->GetVisibility(),
		          ESlateVisibility::Collapsed);
		TestEqual(TEXT("Switching back restores real time text"),
		          CountdownText->GetText().ToString(),
		          FString(TEXT("00:15")));
		TestEqual(TEXT("Switching back restores countdown"),
		          CountdownText->GetVisibility(),
		          ESlateVisibility::HitTestInvisible);
		TestEqual(TEXT("Normal countdown still controls the needle"), ClockNeedle->GetRenderTransform().Angle, 0.0f);
	}
#if WITH_EDITOR
	UReEchoEncounterHudWidget* Preview = NewObject<UReEchoEncounterHudWidget>(GetTransientPackage(), EncounterHudClass);
	Preview->Initialize();
	Preview->SetDesignerFlags(EWidgetDesignFlags::Designing | EWidgetDesignFlags::ExecutePreConstruct);
	Preview->bPreviewBossEncounter = true;
	Preview->PreviewBossHealthRatio = 0.25f;
	Preview->TakeWidget();
	UReEchoBossHealthArcWidget* PreviewArc =
	    Cast<UReEchoBossHealthArcWidget>(Preview->GetWidgetFromName(TEXT("BossHealthArc")));
	UImage* PreviewNeedle = Cast<UImage>(Preview->GetWidgetFromName(TEXT("ArtClockNeedle")));
	UTextBlock* PreviewPercent = Cast<UTextBlock>(Preview->GetWidgetFromName(TEXT("BossHealthPercentText")));
	TestNotNull(TEXT("Designer percentage preview exists"), PreviewPercent);
	if (PreviewPercent)
	{
		TestEqual(TEXT("Designer percentage previews the same health as the arc"),
		          PreviewPercent->GetText().ToString(),
		          FString(TEXT("25%")));
	}
	TestNotNull(TEXT("Designer preview arc exists"), PreviewArc);
	TestNotNull(TEXT("Designer preview needle exists"), PreviewNeedle);
	if (PreviewArc && PreviewNeedle)
	{
		TestEqual(TEXT("Designer preview uses authored preview health"), PreviewArc->GetHealthRatio(), 0.25f);
		TestEqual(TEXT("Designer preview needle agrees with arc"), PreviewNeedle->GetRenderTransform().Angle, -45.0f);
	}
#endif
	TArray<UWidget*> EncounterWidgets;
	AuthoredEncounterHud->WidgetTree->GetAllWidgets(EncounterWidgets);
	const bool bHasRealMinimap = EncounterWidgets.ContainsByPredicate(
	    [](const UWidget* Widget)
	    {
		    return Cast<UReEchoMinimapCanvasWidget>(Widget) != nullptr;
	    });
	TestTrue(TEXT("Encounter HUD retains the real minimap widget"), bHasRealMinimap);
	TestNull(TEXT("Rejected bottom panel is absent from the Encounter HUD"),
	         AuthoredEncounterHud->GetWidgetFromName(TEXT("ArtSkillBar")));

	const TCHAR* CharacterProfiles[] = {
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_HEART.DA_Character_J_HEART"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_SPADE.DA_Character_J_SPADE"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_CLOVER.DA_Character_J_CLOVER"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Character_J_DIAMOND.DA_Character_J_DIAMOND"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_HEART.DA_Echo_J_HEART"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_SPADE.DA_Echo_J_SPADE"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_CLOVER.DA_Echo_J_CLOVER"),
	    TEXT("/Game/ReEcho/DataAsset/Character/Profiles/DA_Echo_J_DIAMOND.DA_Echo_J_DIAMOND"),
	};
	for (const TCHAR* ProfilePath : CharacterProfiles)
	{
		const UReEcho2DCharacterPresentationProfile* Profile =
		    LoadObject<UReEcho2DCharacterPresentationProfile>(nullptr, ProfilePath);
		const FString ProfileLoadsWhat = FString::Printf(TEXT("Minimap profile loads: %s"), ProfilePath);
		TestNotNull(*ProfileLoadsWhat, Profile);
		if (Profile)
		{
			const FString ProfileOwnsIconWhat = FString::Printf(TEXT("Minimap profile owns an icon: %s"), ProfilePath);
			TestNotNull(*ProfileOwnsIconWhat, Profile->MinimapIcon.Get());
		}
	}

	// 暴击伤害：CriticalMultiplier 仍属于结算透传契约，但不再驱动跳字视觉倍率。
	FReEchoHitIntent CriticalIntent;
	CriticalIntent.bCritical = true;
	CriticalIntent.CriticalMultiplier = 1.5f;
	FReEchoHitResolved CriticalResolved;
	CriticalResolved.bCritical = CriticalIntent.bCritical;
	CriticalResolved.CriticalMultiplier = CriticalIntent.CriticalMultiplier;
	FReEchoDamageEvent CriticalEvent;
	CriticalEvent.bCritical = CriticalResolved.bCritical;
	CriticalEvent.CriticalMultiplier = CriticalResolved.CriticalMultiplier;
	TestTrue(TEXT("Crit damage event preserves bCritical"), CriticalEvent.bCritical);
	TestEqual(TEXT("Crit damage event carries CriticalMultiplier"), CriticalEvent.CriticalMultiplier, 1.5f);

	FReEchoDamageEvent OrdinaryEvent;
	TestFalse(TEXT("Non-crit damage event is not critical"), OrdinaryEvent.bCritical);
	TestEqual(TEXT("Non-crit damage event defaults CriticalMultiplier to 1.0"), OrdinaryEvent.CriticalMultiplier, 1.0f);
	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReEchoBossHealthArcRenderTest,
                                 "ReEcho.UI.CombatHud.BossArcRendering",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter |
                                     EAutomationTestFlags::NonNullRHI)

bool FReEchoBossHealthArcRenderTest::RunTest(const FString& Parameters)
{
	UClass* HudClass = LoadClass<UReEchoEncounterHudWidget>(
	    nullptr, TEXT("/Game/ReEcho/UI/WBP_ReEchoEncounterHud.WBP_ReEchoEncounterHud_C"));
	if (!TestNotNull(TEXT("HUD render class loads"), HudClass))
	{
		return false;
	}
	UReEchoEncounterHudWidget* Hud = NewObject<UReEchoEncounterHudWidget>(GetTransientPackage(), HudClass);
	Hud->Initialize();
	FAssetCompilingManager::Get().FinishAllCompilation();
	UReEchoBossHealthArcWidget* Arc = Cast<UReEchoBossHealthArcWidget>(Hud->GetWidgetFromName(TEXT("BossHealthArc")));
	if (!TestNotNull(TEXT("Authored health arc is bound"), Arc) ||
	    !TestNotNull(TEXT("Arc has an authored UI material"), Arc->ArcMaterial.Get()))
	{
		return false;
	}
	// UE 5.8 defers material shader jobs until requested by drawing. This
	// synchronous Windows pixel test must submit those jobs before taking a frame.
	bool bCompiledPlatform = false;
	for (const EShaderPlatform Platform : {SP_PCD3D_SM5, SP_PCD3D_SM6})
	{
		if (FMaterialResource* Resource = Arc->ArcMaterial->GetMaterial()->GetMaterialResource(Platform))
		{
			Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
			FAssetCompilingManager::Get().FinishAllCompilation();
			bCompiledPlatform |= Resource->IsGameThreadShaderMapComplete();
			TestTrue(TEXT("Arc shader compiles without errors"), Resource->GetCompileErrors().IsEmpty());
		}
	}
	if (!TestTrue(TEXT("Windows arc shader is ready for GPU rendering"), bCompiledPlatform))
	{
		return false;
	}
	const FSlateColorBrush Background(FLinearColor(0.04f, 0.035f, 0.03f, 1.0f));
	TSharedRef<SWidget> Canvas = SNew(SBorder).BorderImage(&Background).Padding(0.0f)[Hud->TakeWidget()];
	FWidgetRenderer Renderer(true, true);
	const FVector2D DrawSize(1920.0, 1080.0);
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/Plan158");
	IFileManager::Get().MakeDirectory(*Directory, true);
	auto ReadWidget = [this, &Renderer](const TSharedRef<SWidget>& Widget, const FVector2D Size)
	{
		TArray<FColor> Pixels;
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Widget, Size);
		if (TestNotNull(TEXT("Surface comparison render target"), Target))
		{
			Renderer.DrawWidget(Target, Widget, Size, 0.0f);
			FReadSurfaceDataFlags Flags;
			Flags.SetLinearToGamma(false);
			TestTrue(TEXT("Surface GPU readback"),
			         Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags));
		}
		return Pixels;
	};
	auto SaveHud = [this, &Directory](const TArray<FColor>& Pixels, const TCHAR* Name)
	{
		if (Pixels.Num() >= 1920 * 320)
		{
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(1920, 320, MakeArrayView(Pixels.GetData(), 1920 * 320), Png);
			TestTrue(TEXT("Authored HUD comparison saved"), FFileHelper::SaveArrayToFile(Png, *(Directory / Name)));
		}
	};
	Hud->SetEncounterStatus(8, 8, 15.0f, 30.0f, true, 1300.0f, 2000.0f);
	SaveHud(ReadWidget(Canvas, DrawSize), TEXT("Authored_Boss_065_Surface.png"));
	const float AuthoredVein = Arc->VeinStrength;
	const float AuthoredRelief = Arc->ReliefStrength;
	Arc->VeinStrength = 0.0f;
	Arc->SynchronizeProperties();
	SaveHud(ReadWidget(Canvas, DrawSize), TEXT("Authored_Boss_065_ReliefOnly.png"));
	Arc->ReliefStrength = 0.0f;
	Arc->SynchronizeProperties();
	SaveHud(ReadWidget(Canvas, DrawSize), TEXT("Authored_Boss_065_Flat.png"));
	Arc->VeinStrength = AuthoredVein;
	Arc->ReliefStrength = AuthoredRelief;
	// A controlled transient palette makes pixel coverage independent of art edits.
	// The two authored screenshots above retain the exact saved colors and alpha.
	Arc->FillColor = FLinearColor(0.68f, 0.025f, 0.18f, 1.0f);
	Arc->EmptyColor = FLinearColor(0.008f, 0.008f, 0.01f, 1.0f);
	Arc->TickContrast = 0.25f;
	Arc->SynchronizeProperties();
	if (UTextBlock* Percent = Cast<UTextBlock>(Hud->GetWidgetFromName(TEXT("BossHealthPercentText"))))
	{
		// An artist may use the same hue for the number and fill. Keep the number
		// visible, but exclude it from this controlled color-coverage measurement.
		Percent->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
	TArray<int32> FillCounts;
	for (const float Ratio : {1.0f, 0.75f, 0.5f, 0.25f, 0.0f, -1.0f})
	{
		Hud->SetEncounterStatus(Ratio < 0.0f ? 1 : 8, 8, 15.0f, 30.0f, Ratio >= 0.0f, Ratio * 2000.0f, 2000.0f);
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Canvas, DrawSize);
		if (!TestNotNull(TEXT("HUD GPU render target exists"), Target))
		{
			return false;
		}
		Renderer.DrawWidget(Target, Canvas, DrawSize, 0.0f);
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags;
		ReadFlags.SetLinearToGamma(false); // FWidgetRenderer already wrote gamma-corrected UI pixels.
		if (!TestTrue(TEXT("Read HUD GPU pixels"),
		              Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags)))
		{
			return false;
		}
		int32 FillPixels = 0;
		for (int32 Y = 104; Y < 240; ++Y)
		{
			for (int32 X = 835; X < 1115; ++X)
			{
				const FColor Pixel = Pixels[Y * 1920 + X];
				FillPixels += Pixel.R > Pixel.B + 15 && Pixel.B > Pixel.G + 15 && Pixel.R > 100 ? 1 : 0;
			}
		}
		FillCounts.Add(FillPixels);
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(1920, 280, MakeArrayView(Pixels.GetData(), 1920 * 280), Png);
		const FString Name =
		    Ratio < 0.0f ? TEXT("Normal") : FString::Printf(TEXT("Boss_%03d"), FMath::RoundToInt(Ratio * 100));
		TestTrue(TEXT("HUD render evidence saved"),
		         FFileHelper::SaveArrayToFile(Png, *(Directory / (Name + TEXT(".png")))));
		AddInfo(FString::Printf(TEXT("%s purple-red fill pixels: %d"), *Name, FillPixels));
	}
	TestTrue(TEXT("Full Boss arc renders visible purple fill"), FillCounts[0] > 100);
	TestTrue(TEXT("Half Boss health renders about half the filled arc"),
	         FillCounts[2] > FillCounts[0] / 4 && FillCounts[2] < FillCounts[0] * 3 / 4);
	TestTrue(TEXT("Filled arc shrinks monotonically"),
	         FillCounts[0] > FillCounts[1] && FillCounts[1] > FillCounts[2] && FillCounts[2] > FillCounts[3]);
	TestTrue(TEXT("Zero health leaves no purple health fill"), FillCounts[4] < FMath::Max(1, FillCounts[0] / 25));
	TestTrue(TEXT("Normal encounter has no Boss health fill"), FillCounts[5] < FMath::Max(1, FillCounts[0] / 25));
	// Isolated translucent arc: verify GPU color changes without changing alpha.
	UReEchoBossHealthArcWidget* Probe = NewObject<UReEchoBossHealthArcWidget>();
	Probe->ArcMaterial = Arc->ArcMaterial;
	Probe->FillColor = FLinearColor(0.4f, 0.1f, 0.55f, 0.7f);
	Probe->EmptyColor = FLinearColor(0.02f, 0.01f, 0.03f, 0.4f);
	const TSharedRef<SWidget> ProbeSlate = Probe->TakeWidget();
	const FVector2D ProbeSize = Probe->ReferenceSize;
	auto CaptureSurface = [&ReadWidget, Probe, &ProbeSlate, ProbeSize](float Ratio, float Vein, float Relief)
	{
		Probe->VeinStrength = Vein;
		Probe->ReliefStrength = Relief;
		Probe->SetHealthRatio(Ratio);
		return ReadWidget(ProbeSlate, ProbeSize);
	};
	auto CompareSurface =
	    [this](const TArray<FColor>& A, const TArray<FColor>& B, const TCHAR* What, bool bExpectDifference)
	{
		if (!TestTrue(TEXT("Surface captures have matching pixels"), !A.IsEmpty() && A.Num() == B.Num()))
		{
			return;
		}
		int32 RGBChanges = 0;
		int32 AlphaChanges = 0;
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			RGBChanges += A[Index].R != B[Index].R || A[Index].G != B[Index].G || A[Index].B != B[Index].B;
			AlphaChanges += A[Index].A != B[Index].A;
		}
		TestEqual(TEXT("Surface leaves every alpha pixel unchanged"), AlphaChanges, 0);
		TestTrue(What, bExpectDifference ? RGBChanges > 100 : RGBChanges == 0);
		AddInfo(FString::Printf(TEXT("%s: RGB changes=%d, alpha changes=%d"), What, RGBChanges, AlphaChanges));
	};
	const TArray<FColor> Flat = CaptureSurface(1.0f, 0.0f, 0.0f);
	const TArray<FColor> Veins = CaptureSurface(1.0f, 0.7f, 0.0f);
	CompareSurface(Flat, Veins, TEXT("Vein strength changes visible detail"), true);
	const TArray<FColor> ReliefOnly = CaptureSurface(1.0f, 0.0f, 0.8f);
	CompareSurface(Flat, ReliefOnly, TEXT("Whole strip remains raised with no veins"), true);
	Probe->VeinWidth = 2.5f;
	CompareSurface(
	    ReliefOnly, CaptureSurface(1.0f, 0.0f, 0.8f), TEXT("Vein width cannot change whole-strip relief"), false);
	CompareSurface(Veins, CaptureSurface(1.0f, 0.7f, 0.0f), TEXT("Vein width changes GPU detail"), true);
	Probe->VeinWidth = 1.3f;
	Probe->VeinSpacing = 75.0f;
	CompareSurface(
	    ReliefOnly, CaptureSurface(1.0f, 0.0f, 0.8f), TEXT("Vein spacing cannot change whole-strip relief"), false);
	CompareSurface(Veins, CaptureSurface(1.0f, 0.7f, 0.0f), TEXT("Vein spacing changes GPU detail"), true);
	CompareSurface(Flat, CaptureSurface(1.0f, 0.0f, 0.0f), TEXT("Zero strengths restore flat appearance"), false);
	CompareSurface(CaptureSurface(0.0f, 0.0f, 0.0f),
	               CaptureSurface(0.0f, 1.0f, 1.0f),
	               TEXT("Empty track is never textured"),
	               false);
	const TArray<FColor> FullSurface = CaptureSurface(1.0f, 0.7f, 0.6f);
	const TArray<FColor> HalfSurface = CaptureSurface(0.5f, 0.7f, 0.6f);
	int32 MovedPixels = 0;
	for (int32 Y = 60; Y < static_cast<int32>(ProbeSize.Y); ++Y)
	{
		for (int32 X = 590; X < static_cast<int32>(ProbeSize.X); ++X)
		{
			const int32 Index = Y * static_cast<int32>(ProbeSize.X) + X;
			if (FullSurface.IsValidIndex(Index) && HalfSurface.IsValidIndex(Index))
			{
				MovedPixels += FullSurface[Index] != HalfSurface[Index];
			}
		}
	}
	TestEqual(TEXT("Remaining right-half texture does not slide when health drops"), MovedPixels, 0);
	return true;
}
#endif

#endif
