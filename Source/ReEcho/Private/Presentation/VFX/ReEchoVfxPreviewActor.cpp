#include "Presentation/VFX/ReEchoVfxPreviewActor.h"

#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Combat/ReEchoCombatantComponent.h"
#include "Combat/ReEchoElementReaction.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Graybox/ReEchoEnemyActor.h"
#include "InputCoreTypes.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Presentation/VFX/ReEchoCombatVfxCatalog.h"
#include "Presentation/VFX/ReEchoCombatVfxComponent.h"
#include "Presentation/VFX/ReEchoElementReactionVfxCatalog.h"
#include "Presentation/Scene/ReEchoArenaCameraActor.h"
#include "Presentation/Weapon/ReEchoWeaponPresentationProfile.h"
#include "Weapons/ReEchoWeaponVisualCatalog.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

AReEchoVfxScenarioTargetActor::AReEchoVfxScenarioTargetActor()
{
	bIsEditorOnlyActor = true;
	SetActorEnableCollision(false);
}

AReEchoVfxPreviewActor::AReEchoVfxPreviewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bIsEditorOnlyActor = true;
	PreviewRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot"));
	SetRootComponent(PreviewRoot);
	SourceAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("Source"));
	SourceAnchor->SetupAttachment(PreviewRoot);
	TargetAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("Target"));
	TargetAnchor->SetupAttachment(PreviewRoot);
	TargetAnchor->SetRelativeLocation(FVector(600.0f, 0.0f, 0.0f));
	PreviewEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PreviewEffect"));
	PreviewEffect->SetupAttachment(PreviewRoot);
	PreviewEffect->SetAutoActivate(false);
	ProductionTargetOffsets = {FVector::ZeroVector,
	                           FVector(-80.0f, 0.0f, 0.0f),
	                           FVector(0.0f, 80.0f, 0.0f),
	                           FVector(80.0f, 0.0f, 0.0f),
	                           FVector(160.0f, 0.0f, 0.0f),
	                           FVector(400.0f, 0.0f, 0.0f)};
}

void AReEchoVfxPreviewActor::BeginPlay()
{
	Super::BeginPlay();
	ClearPreview();
	SetActorEnableCollision(false);
	if (bRunProductionConductInPie && (ScenarioPreset == EReEchoVfxScenarioPreset::Conduct2Targets ||
	                                   ScenarioPreset == EReEchoVfxScenarioPreset::ConductChain))
	{
		RunProductionConductPie();
	}
	else
	{
		Destroy();
	}
}

void AReEchoVfxPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPreview();
	RemoveProductionControls();
	for (AActor* Actor : ProductionScenarioActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	ProductionScenarioActors.Reset();
	if (ScenarioCamera)
	{
		ScenarioCamera->Destroy();
		ScenarioCamera = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AReEchoVfxPreviewActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateScenarioCamera(DeltaSeconds);
	DrawScenarioDebug();
}

FVector AReEchoVfxPreviewActor::ResolveSafeDirection(const FVector& Source, const FVector& Target)
{
	const FVector Direction = (Target - Source).GetSafeNormal2D();
	return Direction.IsNearlyZero() ? FVector::ForwardVector : Direction;
}

FTransform AReEchoVfxPreviewActor::ResolvePreviewTransform(const FVector& Source,
                                                           const FVector& Target,
                                                           const FVector& AuthoredAxis,
                                                           const FTransform& Sandbox)
{
	const FVector SafeAxis =
	    AuthoredAxis.GetSafeNormal2D().IsNearlyZero() ? FVector::ForwardVector : AuthoredAxis.GetSafeNormal2D();
	FRotator Rotation = ResolveSafeDirection(Source, Target).Rotation();
	Rotation.Yaw -= SafeAxis.Rotation().Yaw;
	return Sandbox * FTransform(Rotation, Source);
}

FString AReEchoVfxPreviewActor::ResolveProductionPath(const EReEchoVfxPreviewMode PreviewMode,
                                                      const uint8 CombatSemanticValue,
                                                      const uint8 ElementSemanticValue,
                                                      const FName TargetId,
                                                      const FName VisualKey,
                                                      const EReEchoVfxPreviewWeaponSlot Slot,
                                                      const TSoftObjectPtr<UNiagaraSystem>& RawNiagara)
{
	if (PreviewMode == EReEchoVfxPreviewMode::RawNiagara)
	{
		return RawNiagara.ToSoftObjectPath().ToString();
	}
	if (PreviewMode == EReEchoVfxPreviewMode::CombatSemantic &&
	    CombatSemanticValue <= static_cast<uint8>(EReEchoCombatVfxSemantic::EnemyHurt))
	{
		return FReEchoCombatVfxCatalog::ResolvePath(static_cast<EReEchoCombatVfxSemantic>(CombatSemanticValue));
	}
	if (PreviewMode == EReEchoVfxPreviewMode::ElementSemantic &&
	    ElementSemanticValue <= static_cast<uint8>(EReEchoElementReactionVfxSemantic::EnhanceWater))
	{
		return FReEchoElementReactionVfxCatalog::ResolvePath(
		    static_cast<EReEchoElementReactionVfxSemantic>(ElementSemanticValue), TargetId);
	}
	if (PreviewMode != EReEchoVfxPreviewMode::WeaponProfile)
	{
		return FString();
	}
	const UReEchoWeaponPresentationProfile* Profile = FReEchoWeaponVisualCatalog::ResolveProfile(VisualKey);
	if (!Profile)
	{
		return FString();
	}
	const FReEchoWeaponVfxSlot* ResolvedSlot = nullptr;
	switch (Slot)
	{
		case EReEchoVfxPreviewWeaponSlot::Charge:
			ResolvedSlot = &Profile->Charge;
			break;
		case EReEchoVfxPreviewWeaponSlot::Travel:
			ResolvedSlot = &Profile->Travel;
			break;
		case EReEchoVfxPreviewWeaponSlot::DamageApplied:
			ResolvedSlot = &Profile->DamageApplied;
			break;
		case EReEchoVfxPreviewWeaponSlot::AttackCommitted:
			ResolvedSlot = &Profile->AttackCommitted;
			break;
	}
	return ResolvedSlot && ResolvedSlot->IsConfigured() ? ResolvedSlot->System.ToSoftObjectPath().ToString()
	                                                    : FString();
}

void AReEchoVfxPreviewActor::RefreshResolvedSelection()
{
	ResolvedAssetPath = ResolveProductionPath(
	    Mode, CombatSemantic, ElementSemantic, ElementTargetId, WeaponVisualKey, WeaponSlot, RawSystem);
	SupportStatus = ResolvedAssetPath.IsEmpty() ? TEXT("Missing") : TEXT("Supported (read-only production mapping)");
	AuthoredForwardAxis =
	    Mode == EReEchoVfxPreviewMode::CombatSemantic &&
	            CombatSemantic <= static_cast<uint8>(EReEchoCombatVfxSemantic::EnemyHurt)
	        ? FReEchoCombatVfxCatalog::ResolveAuthoredForwardAxis(static_cast<EReEchoCombatVfxSemantic>(CombatSemantic))
	        : FVector::ForwardVector;
}

void AReEchoVfxPreviewActor::PlayPreview()
{
	ClearPreview();
	RefreshResolvedSelection();
	if (ResolvedAssetPath.IsEmpty())
	{
		return;
	}
	UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *ResolvedAssetPath);
	if (!System)
	{
		SupportStatus = TEXT("Missing (asset failed to load)");
		return;
	}
	PreviewEffect->SetAsset(System);
	const FTransform Transform = ResolvePreviewTransform(SourceAnchor->GetComponentLocation(),
	                                                     TargetAnchor->GetComponentLocation(),
	                                                     AuthoredForwardAxis,
	                                                     SandboxTransform);
	PreviewEffect->SetWorldTransform(Transform);
	PreviewEffect->SetTranslucentSortPriority(SortPriority);
	PreviewEffect->Activate(true);
}

void AReEchoVfxPreviewActor::StopPreview()
{
	PreviewEffect->DeactivateImmediate();
}

void AReEchoVfxPreviewActor::RestartPreview()
{
	PlayPreview();
}

void AReEchoVfxPreviewActor::ClearPreview()
{
	PreviewEffect->DeactivateImmediate();
	PreviewEffect->SetAsset(nullptr);
	for (UNiagaraComponent* Effect : AdditionalPreviewEffects)
	{
		if (Effect)
		{
			Effect->DeactivateImmediate();
			Effect->DestroyComponent();
		}
	}
	AdditionalPreviewEffects.Reset();
}

void AReEchoVfxPreviewActor::FaceTarget()
{
	RefreshResolvedSelection();
	PreviewEffect->SetWorldTransform(ResolvePreviewTransform(SourceAnchor->GetComponentLocation(),
	                                                         TargetAnchor->GetComponentLocation(),
	                                                         AuthoredForwardAxis,
	                                                         SandboxTransform));
}

void AReEchoVfxPreviewActor::SwapSourceAndTarget()
{
	const FVector Source = SourceAnchor->GetRelativeLocation();
	SourceAnchor->SetRelativeLocation(TargetAnchor->GetRelativeLocation());
	TargetAnchor->SetRelativeLocation(Source);
	RestartPreview();
}

void AReEchoVfxPreviewActor::SpawnDirectionSet(const int32 DirectionCount)
{
	ClearPreview();
	RefreshResolvedSelection();
	UNiagaraSystem* System =
	    ResolvedAssetPath.IsEmpty() ? nullptr : LoadObject<UNiagaraSystem>(nullptr, *ResolvedAssetPath);
	if (!System || DirectionCount <= 0)
	{
		return;
	}
	const FVector Source = SourceAnchor->GetComponentLocation();
	const float Radius = FMath::Max(100.0f, ProjectileDistanceCm * 0.5f);
	for (int32 DirectionIndex = 0; DirectionIndex < DirectionCount; ++DirectionIndex)
	{
		const float Radians = 2.0f * UE_PI * static_cast<float>(DirectionIndex) / static_cast<float>(DirectionCount);
		const FVector Target = Source + FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0f) * Radius;
		UNiagaraComponent* Effect = NewObject<UNiagaraComponent>(this);
		Effect->SetupAttachment(PreviewRoot);
		Effect->RegisterComponent();
		Effect->SetAsset(System);
		Effect->SetWorldTransform(ResolvePreviewTransform(Source, Target, AuthoredForwardAxis, SandboxTransform));
		Effect->SetTranslucentSortPriority(SortPriority);
		Effect->Activate(true);
		AdditionalPreviewEffects.Add(Effect);
	}
}

void AReEchoVfxPreviewActor::FourDirections()
{
	SpawnDirectionSet(4);
}

void AReEchoVfxPreviewActor::EightDirections()
{
	SpawnDirectionSet(8);
}

void AReEchoVfxPreviewActor::ProjectilePreview()
{
	const FVector Direction =
	    ResolveSafeDirection(SourceAnchor->GetComponentLocation(), TargetAnchor->GetComponentLocation());
	TargetAnchor->SetWorldLocation(SourceAnchor->GetComponentLocation() +
	                               Direction * FMath::Max(1.0f, ProjectileDistanceCm));
	PlayPreview();
}

void AReEchoVfxPreviewActor::GenerateCalibrationReport()
{
	RefreshResolvedSelection();
	CalibrationReport = FString::Printf(
	    TEXT("UNAPPLIED PREVIEW | Mode=%d | Asset=%s | AuthoredAxis=%s | Offset=%s | Rotation=%s | Scale=%s | "
	         "Sort=%d | ProjectileSpeed=%.1f | ProductionOwner=Catalog/Profile; gameplay lifecycle unchanged"),
	    static_cast<int32>(Mode),
	    *ResolvedAssetPath,
	    *AuthoredForwardAxis.ToCompactString(),
	    *SandboxTransform.GetLocation().ToCompactString(),
	    *SandboxTransform.Rotator().ToCompactString(),
	    *SandboxTransform.GetScale3D().ToCompactString(),
	    SortPriority,
	    ProjectileSpeedCmPerSecond);
}

void AReEchoVfxPreviewActor::ResetSandbox()
{
	SandboxTransform = FTransform::Identity;
	SortPriority = 1000;
	ProjectileDistanceCm = 600.0f;
	ProjectileSpeedCmPerSecond = 500.0f;
	RestartPreview();
}

TArray<FReEchoElementReactionLink>
AReEchoVfxPreviewActor::ConsumeResolvedReactionLinks(const FReEchoElementReactionResolvedEvent& Event)
{
	return Event.ReactionLinks;
}

TArray<FReEchoElementReactionLink>
AReEchoVfxPreviewActor::ConvertAuthoredReactionLinks(const TArray<FReEchoVfxScenarioLink>& AuthoredLinks)
{
	TArray<FReEchoElementReactionLink> Result;
	Result.Reserve(AuthoredLinks.Num());
	for (const FReEchoVfxScenarioLink& Authored : AuthoredLinks)
	{
		FReEchoElementReactionLink& Link = Result.AddDefaulted_GetRef();
		Link.SourceTarget = Authored.Source;
		Link.TargetTarget = Authored.Target;
	}
	return Result;
}

void AReEchoVfxPreviewActor::RunScenario()
{
	ClearPreview();
	if (ScenarioPreset == EReEchoVfxScenarioPreset::Conduct2Targets ||
	    ScenarioPreset == EReEchoVfxScenarioPreset::ConductChain)
	{
		SupportStatus = TEXT("Production Conduct is PIE-only; editor Run shows Visual Calibration (NOT APPLIED)");
	}
	else if (ScenarioPreset == EReEchoVfxScenarioPreset::Projectile)
	{
		ProjectilePreview();
	}
	else
	{
		PlayPreview();
	}
	GenerateScenarioReport();
}

void AReEchoVfxPreviewActor::PauseScenario()
{
	PreviewEffect->SetPaused(!PreviewEffect->IsPaused());
	for (UNiagaraComponent* Effect : AdditionalPreviewEffects)
	{
		if (Effect)
		{
			Effect->SetPaused(!Effect->IsPaused());
		}
	}
}

void AReEchoVfxPreviewActor::StepScenario()
{
	PreviewEffect->SetPaused(true);
	PreviewEffect->AdvanceSimulation(1, FMath::Max(0.001f, PreviewStepSeconds));
}

void AReEchoVfxPreviewActor::RestartScenario()
{
	RunScenario();
}

void AReEchoVfxPreviewActor::ResetScenario()
{
	ClearPreview();
	for (AReEchoVfxScenarioTargetActor* Target : CalibrationTargets)
	{
		if (Target && Target->GetOwner() == this)
		{
			Target->Destroy();
		}
	}
	CalibrationTargets.Reset();
	ResolvedReactionEvent = FReEchoElementReactionResolvedEvent();
	AuthoredReactionLinks.Reset();
	ScenarioPreset = EReEchoVfxScenarioPreset::SingleHit;
	ScenarioRadiusCm = 500.0f;
	bRadiusVisible = false;
	bDirectionVisible = true;
	ScenarioReport.Reset();
}

void AReEchoVfxPreviewActor::AddScenarioTarget()
{
	if (!GetWorld())
	{
		return;
	}
	FActorSpawnParameters Parameters;
	Parameters.Owner = this;
	AReEchoVfxScenarioTargetActor* Target = GetWorld()->SpawnActor<AReEchoVfxScenarioTargetActor>(
	    GetActorLocation() + FVector(300.0f, CalibrationTargets.Num() * 180.0f, 0.0f),
	    FRotator::ZeroRotator,
	    Parameters);
	if (Target)
	{
		Target->TargetId = FName(*FString::Printf(TEXT("Preview.Target.%d"), CalibrationTargets.Num()));
		Target->ConductOrder = CalibrationTargets.Num();
		CalibrationTargets.Add(Target);
	}
}

void AReEchoVfxPreviewActor::RemoveScenarioTarget()
{
	if (CalibrationTargets.IsEmpty())
	{
		return;
	}
	AActor* Target = CalibrationTargets.Pop();
	AuthoredReactionLinks.RemoveAll(
	    [Target](const FReEchoVfxScenarioLink& Link)
	    {
		    return Link.Source == Target || Link.Target == Target;
	    });
	if (Target && Target->GetOwner() == this)
	{
		Target->Destroy();
	}
}

void AReEchoVfxPreviewActor::ShowRadius()
{
	bRadiusVisible = !bRadiusVisible;
}

void AReEchoVfxPreviewActor::ShowDirection()
{
	bDirectionVisible = !bDirectionVisible;
}

void AReEchoVfxPreviewActor::RunProductionConductPie()
{
	if (!GetWorld() || GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}
	InitializeProductionScenario();
	CreateProductionControls();
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		EnableInput(Controller);
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AReEchoVfxPreviewActor::ReleaseProductionVfx);
			InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AReEchoVfxPreviewActor::RestartProductionScenario);
			InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AReEchoVfxPreviewActor::ResetProductionTargets);
			InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AReEchoVfxPreviewActor::ClearProductionScenario);
			InputComponent->BindKey(EKeys::Home, IE_Pressed, this, &AReEchoVfxPreviewActor::ResetScenarioCamera);
		}
	}
	if (bAutoReleaseOnBeginPlay)
	{
		ReleaseProductionVfx();
	}
}

void AReEchoVfxPreviewActor::InitializeProductionScenario()
{
	ClearProductionScenario();
	for (int32 Index = 0; Index < ProductionTargetOffsets.Num(); ++Index)
	{
		AReEchoEnemyActor* Enemy = GetWorld()->SpawnActor<AReEchoEnemyActor>(
		    GetActorLocation() + ProductionTargetOffsets[Index], FRotator::ZeroRotator);
		if (!Enemy)
		{
			continue;
		}
		Enemy->Configure(EReEchoEnemyKind::Grunt, 9000 + Index);
		FReEchoStatBlock Stats;
		Stats.HpMax = 1000.0f;
		Stats.HpPoint = 1000.0f;
		Enemy->GetCombatantComponent()->BindToAbilitySystem(Enemy->GetAbilitySystemComponent());
		Enemy->GetCombatantComponent()->InitializeFromStats(Stats, true);
		ProductionScenarioActors.Add(Enemy);
	}
	ProductionStatus = ProductionScenarioActors.IsEmpty() ? TEXT("Missing") : TEXT("Ready");
	ResolvedReactionEvent = FReEchoElementReactionResolvedEvent();
	if (!ScenarioCamera && GetWorld())
	{
		ScenarioCamera = GetWorld()->SpawnActor<AReEchoArenaCameraActor>(CameraPan, CameraRotation);
		if (ScenarioCamera && ScenarioCamera->ArenaCamera)
		{
			ScenarioCamera->ArenaCamera->SetOrthoWidth(ClampScenarioOrthoWidth(CameraOrthoWidth));
			if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
			{
				Controller->SetViewTarget(ScenarioCamera);
			}
		}
	}
}

void AReEchoVfxPreviewActor::ReleaseProductionVfx()
{
	if (!GetWorld() || GetWorld()->WorldType != EWorldType::PIE)
	{
		ProductionStatus = TEXT("NOT APPLIED");
		return;
	}
	InitializeProductionScenario();
	AReEchoEnemyActor* Primary =
	    ProductionScenarioActors.IsEmpty() ? nullptr : Cast<AReEchoEnemyActor>(ProductionScenarioActors[0]);
	if (!Primary)
	{
		ProductionStatus = TEXT("Missing");
		return;
	}
	if (UReEchoCombatEventsComponent* Events = Primary->FindComponentByClass<UReEchoCombatEventsComponent>())
	{
		Events->OnElementReactionResolved.AddDynamic(this, &AReEchoVfxPreviewActor::CaptureProductionReaction);
	}
	FReEchoElementHitContext Context;
	Context.SourceLocation = GetActorLocation();
	Context.Attack.Source = this;
	Context.Attack.Sequence = ReleaseCount + 1;
	Context.Attack.WeaponId = ProductionWeaponId;
	ResolvedConductDelaySeconds =
	    UReEchoCombatVfxComponent::ResolveConductPropagationDelaySeconds(Context.Attack.WeaponId);
	for (AActor* Actor : ProductionScenarioActors)
	{
		if (AReEchoEnemyActor* Enemy = Cast<AReEchoEnemyActor>(Actor))
		{
			ReEchoElementReaction::ApplyHitToWorld(*Enemy, EReEchoElement::Water, 0.0f, Context);
		}
	}
	ReEchoElementReaction::ApplyHitToWorld(*Primary, EReEchoElement::Lightning, 0.0f, Context);
	++ReleaseCount;
	ProductionStatus = ResolvedReactionEvent.ReactionId.IsNone() ? TEXT("Missing") : TEXT("Playing");
}

void AReEchoVfxPreviewActor::RestartProductionScenario()
{
	ReleaseCount = 0;
	InitializeProductionScenario();
	ReleaseProductionVfx();
}

void AReEchoVfxPreviewActor::ResetProductionTargets()
{
	InitializeProductionScenario();
}

void AReEchoVfxPreviewActor::ClearProductionScenario()
{
	for (AActor* Actor : ProductionScenarioActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
	ProductionScenarioActors.Reset();
	ResolvedReactionEvent = FReEchoElementReactionResolvedEvent();
	ProductionStatus = TEXT("Ready");
}

void AReEchoVfxPreviewActor::CaptureProductionReaction(const FReEchoElementReactionResolvedEvent& Event)
{
	ResolvedReactionEvent = Event;
	ScenarioRadiusCm = Event.RadiusCm;
	ProductionStatus = TEXT("Playing");
	GenerateScenarioReport();
}

void AReEchoVfxPreviewActor::CreateProductionControls()
{
	if (ProductionControls || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	ProductionControls = SNew(SBorder).Padding(
	    12)[SNew(SVerticalBox) +
	        SVerticalBox::Slot().AutoHeight()
	            [SNew(STextBlock)
	                 .Text_Lambda(
	                     [this]
	                     {
		                     return FText::FromString(FString::Printf(
		                         TEXT("VFX Scenario %d | Releases %d | Targets %d | Links %d | Delay %.2fs | %s"),
		                         static_cast<int32>(ScenarioPreset),
		                         ReleaseCount,
		                         ResolvedReactionEvent.AffectedTargets.Num(),
		                         ResolvedReactionEvent.ReactionLinks.Num(),
		                         ResolvedConductDelaySeconds,
		                         *ProductionStatus));
	                     })] +
	        SVerticalBox::Slot().AutoHeight()[SNew(SButton)
	                                              .Text(FText::FromString(TEXT("释放特效 / Release VFX  [Space]")))
	                                              .OnClicked_Lambda(
	                                                  [this]
	                                                  {
		                                                  ReleaseProductionVfx();
		                                                  return FReply::Handled();
	                                                  })] +
	        SVerticalBox::Slot().AutoHeight()[SNew(SButton)
	                                              .Text(FText::FromString(TEXT("Restart [R]")))
	                                              .OnClicked_Lambda(
	                                                  [this]
	                                                  {
		                                                  RestartProductionScenario();
		                                                  return FReply::Handled();
	                                                  })] +
	        SVerticalBox::Slot().AutoHeight()[SNew(SButton)
	                                              .Text(FText::FromString(TEXT("Reset Targets [T]")))
	                                              .OnClicked_Lambda(
	                                                  [this]
	                                                  {
		                                                  ResetProductionTargets();
		                                                  return FReply::Handled();
	                                                  })] +
	        SVerticalBox::Slot().AutoHeight()[SNew(SButton)
	                                              .Text(FText::FromString(TEXT("Clear [C]")))
	                                              .OnClicked_Lambda(
	                                                  [this]
	                                                  {
		                                                  ClearProductionScenario();
		                                                  return FReply::Handled();
	                                                  })] +
	        SVerticalBox::Slot().AutoHeight()[SNew(SButton)
	                                              .Text(FText::FromString(TEXT("Reset Camera [Home]")))
	                                              .OnClicked_Lambda(
	                                                  [this]
	                                                  {
		                                                  ResetScenarioCamera();
		                                                  return FReply::Handled();
	                                                  })] +
	        SVerticalBox::Slot().AutoHeight()
	            [SNew(STextBlock).Text(FText::FromString(TEXT("Camera: WASD pan | Q/E rotate | Mouse Wheel zoom")))]];
	GEngine->GameViewport->AddViewportWidgetContent(ProductionControls.ToSharedRef(), 1000);
}

void AReEchoVfxPreviewActor::RemoveProductionControls()
{
	if (ProductionControls && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ProductionControls.ToSharedRef());
	}
	ProductionControls.Reset();
}

void AReEchoVfxPreviewActor::ResetScenarioCamera()
{
	CameraPan = FVector(-900.0f, 0.0f, 900.0f);
	CameraRotation = FRotator(-45.0f, 0.0f, 0.0f);
	CameraOrthoWidth = 2560.0f;
	if (ScenarioCamera && ScenarioCamera->ArenaCamera)
	{
		ScenarioCamera->SetActorLocation(CameraPan);
		ScenarioCamera->SetActorRotation(CameraRotation);
		ScenarioCamera->ArenaCamera->SetOrthoWidth(CameraOrthoWidth);
	}
}

void AReEchoVfxPreviewActor::UpdateScenarioCamera(const float DeltaSeconds)
{
	if (!ScenarioCamera || !ScenarioCamera->ArenaCamera || !GetWorld())
	{
		return;
	}
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (!Controller)
	{
		return;
	}
	const float Forward =
	    (Controller->IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f) - (Controller->IsInputKeyDown(EKeys::S) ? 1.0f : 0.0f);
	const float Right =
	    (Controller->IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f) - (Controller->IsInputKeyDown(EKeys::A) ? 1.0f : 0.0f);
	const float Yaw =
	    (Controller->IsInputKeyDown(EKeys::E) ? 1.0f : 0.0f) - (Controller->IsInputKeyDown(EKeys::Q) ? 1.0f : 0.0f);
	CameraPan += FVector(Forward, Right, 0.0f) * CameraMoveSpeed * DeltaSeconds;
	CameraRotation.Yaw += Yaw * 60.0f * DeltaSeconds;
	CameraOrthoWidth = ClampScenarioOrthoWidth(
	    CameraOrthoWidth - Controller->GetInputAnalogKeyState(EKeys::MouseWheelAxis) * CameraZoomSpeed);
	ScenarioCamera->SetActorLocation(CameraPan);
	ScenarioCamera->SetActorRotation(CameraRotation);
	ScenarioCamera->ArenaCamera->SetOrthoWidth(CameraOrthoWidth);
}

void AReEchoVfxPreviewActor::DrawScenarioDebug() const
{
	if (!GetWorld())
	{
		return;
	}
	const FVector Center = ResolvedReactionEvent.PrimaryTarget ? ResolvedReactionEvent.PrimaryTarget->GetActorLocation()
	                                                           : GetActorLocation();
	if (bRadiusVisible)
	{
		DrawDebugCircle(GetWorld(),
		                Center,
		                ScenarioRadiusCm,
		                64,
		                FColor::Cyan,
		                false,
		                0.0f,
		                0,
		                3.0f,
		                FVector::ForwardVector,
		                FVector::RightVector,
		                false);
	}
	if (!bDirectionVisible)
	{
		return;
	}
	const bool bProduction = !ResolvedReactionEvent.ReactionLinks.IsEmpty();
	const TArray<FReEchoElementReactionLink> Links =
	    bProduction ? ResolvedReactionEvent.ReactionLinks : ConvertAuthoredReactionLinks(AuthoredReactionLinks);
	for (const FReEchoElementReactionLink& Link : Links)
	{
		if (!Link.SourceTarget || !Link.TargetTarget)
		{
			continue;
		}
		const IReEchoCombatTarget* SourceCombatTarget = Cast<IReEchoCombatTarget>(Link.SourceTarget);
		const IReEchoCombatTarget* TargetCombatTarget = Cast<IReEchoCombatTarget>(Link.TargetTarget);
		const FVector SourceLocation =
		    SourceCombatTarget ? SourceCombatTarget->GetCombatTargetLocation() : Link.SourceTarget->GetActorLocation();
		const FVector TargetLocation =
		    TargetCombatTarget ? TargetCombatTarget->GetCombatTargetLocation() : Link.TargetTarget->GetActorLocation();
		DrawDebugSphere(
		    GetWorld(), SourceLocation, 12.0f, 12, bProduction ? FColor::Green : FColor::Yellow, false, 0.0f);
		DrawDebugSphere(GetWorld(), TargetLocation, 12.0f, 12, bProduction ? FColor::Red : FColor::Yellow, false, 0.0f);
		DrawDebugDirectionalArrow(GetWorld(),
		                          SourceLocation,
		                          TargetLocation,
		                          35.0f,
		                          bProduction ? FColor::Cyan : FColor::Yellow,
		                          false,
		                          0.0f,
		                          0,
		                          4.0f);
	}
}

void AReEchoVfxPreviewActor::GenerateScenarioReport()
{
	const FString ConductPath =
	    FReEchoElementReactionVfxCatalog::ResolvePath(EReEchoElementReactionVfxSemantic::Conduct);
	TArray<FString> Lines;
	const TCHAR* Authority = ResolvedReactionEvent.ReactionId.IsNone() ? TEXT("NOT APPLIED") : TEXT("PRODUCTION PIE");
	Lines.Add(FString::Printf(TEXT("%s | Preset=%d | Radius=%.1f | Asset=%s | Links=%d | ProductionDelay=%.3f | "
	                               "CalibrationDelayOverride=%.3f NOT APPLIED"),
	                          Authority,
	                          static_cast<int32>(ScenarioPreset),
	                          ScenarioRadiusCm,
	                          *ConductPath,
	                          ResolvedReactionEvent.ReactionLinks.Num(),
	                          ResolvedConductDelaySeconds,
	                          CalibrationConductDelayOverrideSeconds));
	for (int32 Index = 0; Index < ResolvedReactionEvent.ReactionLinks.Num(); ++Index)
	{
		const FReEchoElementReactionLink& Link = ResolvedReactionEvent.ReactionLinks[Index];
		const FVector Start = Link.SourceTarget ? Link.SourceTarget->GetActorLocation() : FVector::ZeroVector;
		const FVector End = Link.TargetTarget ? Link.TargetTarget->GetActorLocation() : FVector::ZeroVector;
		Lines.Add(FString::Printf(TEXT("Link[%d] Source=%s Target=%s Length=%.1f Direction=%s"),
		                          Index,
		                          *GetNameSafe(Link.SourceTarget),
		                          *GetNameSafe(Link.TargetTarget),
		                          FVector::Distance(Start, End),
		                          *(End - Start).GetSafeNormal().ToCompactString()));
	}
	ScenarioReport = FString::Join(Lines, TEXT("\n"));
}
