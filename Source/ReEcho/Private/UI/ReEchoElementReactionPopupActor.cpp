#include "UI/ReEchoElementReactionPopupActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Combat/ReEchoCombatContracts.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace ReEchoElementReactionPopup
{
constexpr TCHAR BurnTexturePath[] =
    TEXT("/Game/ReEcho/Textures/UI/CombatHud/ElementReactions/T_UI_Reaction_Burn.T_UI_Reaction_Burn");
constexpr TCHAR VaporizeTexturePath[] =
    TEXT("/Game/ReEcho/Textures/UI/CombatHud/ElementReactions/T_UI_Reaction_Vaporize.T_UI_Reaction_Vaporize");
constexpr TCHAR GrowthTexturePath[] =
    TEXT("/Game/ReEcho/Textures/UI/CombatHud/ElementReactions/T_UI_Reaction_Growth.T_UI_Reaction_Growth");
constexpr TCHAR ConductTexturePath[] =
    TEXT("/Game/ReEcho/Textures/UI/CombatHud/ElementReactions/T_UI_Reaction_Conduct.T_UI_Reaction_Conduct");
constexpr TCHAR EnhanceTexturePath[] =
    TEXT("/Game/ReEcho/Textures/UI/CombatHud/ElementReactions/T_UI_Reaction_Enhance.T_UI_Reaction_Enhance");
constexpr TCHAR MaterialPath[] =
    TEXT("/Game/ReEcho/Materials/UI/ElementReactions/M_ElementReactionPopup.M_ElementReactionPopup");
constexpr TCHAR BlueprintClassPath[] =
    TEXT("/Game/ReEcho/UI/CombatHud/BP_ReEchoElementReactionPopup.BP_ReEchoElementReactionPopup_C");
const FName TextureParameter = TEXT("ReactionTexture");
const FName OpacityParameter = TEXT("Opacity");
} // namespace ReEchoElementReactionPopup

const TCHAR* AReEchoElementReactionPopupActor::GetReactionTexturePath(const FName ReactionBehaviorId)
{
	if (ReactionBehaviorId == TEXT("Reaction.Burn"))
	{
		return ReEchoElementReactionPopup::BurnTexturePath;
	}
	if (ReactionBehaviorId == TEXT("Reaction.Vaporize"))
	{
		return ReEchoElementReactionPopup::VaporizeTexturePath;
	}
	if (ReactionBehaviorId == TEXT("Reaction.Growth"))
	{
		return ReEchoElementReactionPopup::GrowthTexturePath;
	}
	if (ReactionBehaviorId == TEXT("Reaction.Conduct"))
	{
		return ReEchoElementReactionPopup::ConductTexturePath;
	}
	if (ReactionBehaviorId == TEXT("Reaction.Enhance"))
	{
		return ReEchoElementReactionPopup::EnhanceTexturePath;
	}
	return nullptr;
}

const TCHAR* AReEchoElementReactionPopupActor::GetPopupMaterialPath()
{
	return ReEchoElementReactionPopup::MaterialPath;
}

const TCHAR* AReEchoElementReactionPopupActor::GetPopupBlueprintClassPath()
{
	return ReEchoElementReactionPopup::BlueprintClassPath;
}

bool AReEchoElementReactionPopupActor::ShouldDisplayForTarget(const FReEchoElementReactionResolvedEvent& Event,
                                                              const AActor* Target)
{
	return Target && Event.PrimaryTarget == Target && GetReactionTexturePath(Event.ReactionBehaviorId) != nullptr;
}

float AReEchoElementReactionPopupActor::CalculateOpacity(const float LifeProgress,
                                                         const float FadeStartFraction,
                                                         const float FadeCurveExponent)
{
	const float SafeProgress = FMath::Clamp(LifeProgress, 0.0f, 1.0f);
	const float SafeFadeStart = FMath::Clamp(FadeStartFraction, 0.0f, 0.95f);
	if (SafeProgress <= SafeFadeStart)
	{
		return 1.0f;
	}
	const float FadeProgress = (SafeProgress - SafeFadeStart) / FMath::Max(1.0f - SafeFadeStart, UE_SMALL_NUMBER);
	return FMath::Pow(1.0f - FMath::Clamp(FadeProgress, 0.0f, 1.0f), FMath::Max(FadeCurveExponent, 0.01f));
}

AReEchoElementReactionPopupActor::AReEchoElementReactionPopupActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Visual = CreateDefaultSubobject<UMaterialBillboardComponent>(TEXT("ReactionVisual"));
	Visual->SetupAttachment(SceneRoot);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);
	Visual->SetTranslucentSortPriority(TranslucentSortPriority);

	static ConstructorHelpers::FObjectFinder<UTexture2D> BurnAsset(ReEchoElementReactionPopup::BurnTexturePath);
	static ConstructorHelpers::FObjectFinder<UTexture2D> VaporizeAsset(ReEchoElementReactionPopup::VaporizeTexturePath);
	static ConstructorHelpers::FObjectFinder<UTexture2D> GrowthAsset(ReEchoElementReactionPopup::GrowthTexturePath);
	static ConstructorHelpers::FObjectFinder<UTexture2D> ConductAsset(ReEchoElementReactionPopup::ConductTexturePath);
	static ConstructorHelpers::FObjectFinder<UTexture2D> EnhanceAsset(ReEchoElementReactionPopup::EnhanceTexturePath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
	    ReEchoElementReactionPopup::MaterialPath);
	BurnTexture = BurnAsset.Object;
	VaporizeTexture = VaporizeAsset.Object;
	GrowthTexture = GrowthAsset.Object;
	ConductTexture = ConductAsset.Object;
	EnhanceTexture = EnhanceAsset.Object;
	PopupMaterial = MaterialAsset.Object;
}

AReEchoElementReactionPopupActor* AReEchoElementReactionPopupActor::SpawnReactionPopup(UWorld* World,
                                                                                       const FVector& WorldLocation,
                                                                                       const FName ReactionBehaviorId)
{
	if (!World || !GetReactionTexturePath(ReactionBehaviorId))
	{
		return nullptr;
	}

	UClass* PopupClass = LoadClass<AReEchoElementReactionPopupActor>(nullptr, GetPopupBlueprintClassPath());
	if (!PopupClass)
	{
		PopupClass = StaticClass();
	}
	AReEchoElementReactionPopupActor* Popup =
	    World->SpawnActor<AReEchoElementReactionPopupActor>(PopupClass, WorldLocation, FRotator::ZeroRotator);
	if (!Popup || !Popup->InitializePopup(ReactionBehaviorId))
	{
		if (Popup)
		{
			Popup->Destroy();
		}
		return nullptr;
	}
	return Popup;
}

UTexture2D* AReEchoElementReactionPopupActor::ResolveReactionTexture(const FName ReactionBehaviorId) const
{
	if (ReactionBehaviorId == TEXT("Reaction.Burn"))
	{
		return BurnTexture;
	}
	if (ReactionBehaviorId == TEXT("Reaction.Vaporize"))
	{
		return VaporizeTexture;
	}
	if (ReactionBehaviorId == TEXT("Reaction.Growth"))
	{
		return GrowthTexture;
	}
	if (ReactionBehaviorId == TEXT("Reaction.Conduct"))
	{
		return ConductTexture;
	}
	return ReactionBehaviorId == TEXT("Reaction.Enhance") ? EnhanceTexture.Get() : nullptr;
}

bool AReEchoElementReactionPopupActor::InitializePopup(const FName ReactionBehaviorId)
{
	UTexture2D* Texture = ResolveReactionTexture(ReactionBehaviorId);
	if (!Texture || !PopupMaterial || !Visual)
	{
		UE_LOG(LogTemp,
		       Warning,
		       TEXT("[ElementReactionPopup] Missing presentation asset for behavior=%s texture=%s material=%s"),
		       *ReactionBehaviorId.ToString(),
		       *GetNameSafe(Texture),
		       *GetNameSafe(PopupMaterial));
		return false;
	}

	RuntimeMaterial = UMaterialInstanceDynamic::Create(PopupMaterial, this);
	if (!RuntimeMaterial)
	{
		return false;
	}
	RuntimeMaterial->SetTextureParameterValue(ReEchoElementReactionPopup::TextureParameter, Texture);
	RuntimeMaterial->SetScalarParameterValue(ReEchoElementReactionPopup::OpacityParameter, 1.0f);
	RefreshBillboard(*Texture);
	StartLocation = GetActorLocation() + FVector(0.0f, 0.0f, SpawnHeightCm);
	SetActorLocation(StartLocation);
	ElapsedTime = 0.0f;
	SetActorScale3D(FVector(StartScale));
	FacePlayerCamera();
	return true;
}

void AReEchoElementReactionPopupActor::RefreshBillboard(UTexture2D& Texture)
{
	const float SafeHeight = FMath::Max(WorldHeightCm, 1.0f);
	const float Width = SafeHeight * static_cast<float>(Texture.GetSizeX()) / FMath::Max(Texture.GetSizeY(), 1);
	FMaterialSpriteElement Element;
	Element.Material = RuntimeMaterial;
	Element.bSizeIsInScreenSpace = false;
	Element.BaseSizeX = Width;
	Element.BaseSizeY = SafeHeight;
	Visual->SetElements({Element});
	Visual->SetTranslucentSortPriority(TranslucentSortPriority);
}

void AReEchoElementReactionPopupActor::FacePlayerCamera()
{
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		SetActorRotation((-Camera->GetCameraRotation().Vector()).Rotation());
	}
}

void AReEchoElementReactionPopupActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	const float LifeProgress = DisplayDuration > 0.0f ? FMath::Clamp(ElapsedTime / DisplayDuration, 0.0f, 1.0f) : 1.0f;
	const float RiseProgress = 1.0f - FMath::Pow(1.0f - LifeProgress, 3.0f);
	SetActorLocation(StartLocation + FVector(0.0f, 0.0f, RiseHeightCm * RiseProgress));
	SetActorScale3D(FVector(FMath::Lerp(StartScale, EndScale, LifeProgress)));
	FacePlayerCamera();
	if (RuntimeMaterial)
	{
		RuntimeMaterial->SetScalarParameterValue(ReEchoElementReactionPopup::OpacityParameter,
		                                         CalculateOpacity(LifeProgress, FadeStartFraction, FadeCurveExponent));
	}
	if (LifeProgress >= 1.0f)
	{
		Destroy();
	}
}
