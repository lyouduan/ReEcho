#include "UI/ReEchoDamageNumberActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
/** 暴击跳字基准世界字号，与普通伤害跳字一致。 */
constexpr float DamageNumberBaseWorldSize = 52.0f;

using FDamageNumberPool = TArray<TWeakObjectPtr<AReEchoDamageNumberActor>>;

TMap<TWeakObjectPtr<UWorld>, FDamageNumberPool>& GetDamageNumberPools()
{
	static TMap<TWeakObjectPtr<UWorld>, FDamageNumberPool> Pools;
	return Pools;
}
}

const TCHAR* AReEchoDamageNumberActor::GetDamageNumberFontPath()
{
	return TEXT("/Game/ReEcho/Fonts/DamageNumbers/F_DamageNumber_MFYuYue_Font.F_DamageNumber_MFYuYue_Font");
}

const TCHAR* AReEchoDamageNumberActor::GetDamageNumberMaterialPath()
{
	return TEXT("/Game/ReEcho/Fonts/DamageNumbers/M_DamageNumberTextOpacity.M_DamageNumberTextOpacity");
}

const TCHAR* AReEchoDamageNumberActor::GetDamageNumberBlueprintClassPath()
{
	return TEXT("/Game/ReEcho/UI/CombatHud/BP_ReEchoDamageNumber.BP_ReEchoDamageNumber_C");
}

void AReEchoDamageNumberActor::GatherPreloadAssetPaths(TArray<FString>& OutPaths)
{
	OutPaths.Add(GetDamageNumberBlueprintClassPath());
	OutPaths.Add(GetDamageNumberFontPath());
	OutPaths.Add(GetDamageNumberMaterialPath());
}

#if WITH_DEV_AUTOMATION_TESTS
int32 AReEchoDamageNumberActor::GetPooledCountForTests(UWorld* World)
{
	const FDamageNumberPool* Pool = GetDamageNumberPools().Find(World);
	int32 ValidCount = 0;
	if (Pool)
	{
		for (const TWeakObjectPtr<AReEchoDamageNumberActor>& Entry : *Pool)
		{
			ValidCount += Entry.IsValid() ? 1 : 0;
		}
	}
	return ValidCount;
}
#endif

AReEchoDamageNumberActor::AReEchoDamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DamageText"));
	SetRootComponent(Text);
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(DamageNumberBaseWorldSize);
	Text->SetXScale(1.0f);
	Text->SetYScale(1.0f);
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->SetCastShadow(false);
	Text->SetTranslucentSortPriority(20);
	static ConstructorHelpers::FObjectFinder<UFont> DamageNumberFont(GetDamageNumberFontPath());
	if (DamageNumberFont.Succeeded())
	{
		Text->SetFont(DamageNumberFont.Object);
	}
	if (UMaterialInterface* TranslucentTextMaterial =
	        LoadObject<UMaterialInterface>(nullptr, GetDamageNumberMaterialPath()))
	{
		Text->SetTextMaterial(TranslucentTextMaterial);
	}
}

void AReEchoDamageNumberActor::SpawnDamageNumber(UWorld* World,
                                                 const FVector& WorldLocation,
                                                 const float Damage,
                                                 const FLinearColor& Color,
                                                 const bool bCritical)
{
	if (!World || Damage <= 0.0f)
	{
		return;
	}

	static TWeakObjectPtr<UClass> CachedDamageNumberClass;
	UClass* DamageNumberClass = CachedDamageNumberClass.Get();
	if (!DamageNumberClass)
	{
		DamageNumberClass = LoadClass<AReEchoDamageNumberActor>(nullptr, GetDamageNumberBlueprintClassPath());
		CachedDamageNumberClass = DamageNumberClass;
	}
	if (!DamageNumberClass)
	{
		DamageNumberClass = StaticClass();
	}

	AReEchoDamageNumberActor* DamageNumber = nullptr;
	FDamageNumberPool& Pool = GetDamageNumberPools().FindOrAdd(World);
	for (int32 Index = Pool.Num() - 1; Index >= 0; --Index)
	{
		AReEchoDamageNumberActor* Candidate = Pool[Index].Get();
		if (!Candidate)
		{
			Pool.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Candidate->IsA(DamageNumberClass))
		{
			DamageNumber = Candidate;
			Pool.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			break;
		}
	}
	const FVector SpawnLocation = WorldLocation + FVector(0.0f, 0.0f, 95.0f);
	if (!DamageNumber)
	{
		DamageNumber = World->SpawnActor<AReEchoDamageNumberActor>(
		    DamageNumberClass, SpawnLocation, FRotator::ZeroRotator);
	}
	if (DamageNumber)
	{
		DamageNumber->SetActorLocationAndRotation(SpawnLocation, FRotator::ZeroRotator);
		DamageNumber->InitializeDamage(Damage, Color, bCritical);
	}
}

void AReEchoDamageNumberActor::InitializeDamage(
    const float Damage, const FLinearColor& Color, const bool bCritical)
{
	ElapsedTime = 0.0f;
	InitialColor = Color;
	const int32 DisplayDamage = FMath::Max(1, FMath::RoundToInt(Damage));
	Text->SetText(FText::FromString(FString::Printf(TEXT("%d"), DisplayDamage)));
	Text->SetWorldSize(DamageNumberBaseWorldSize * (bCritical ? CriticalSizeScale : 1.0f));
	Text->SetTextRenderColor(InitialColor.ToFColor(false));
	SetActorScale3D(FVector(StartScale));
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
}

void AReEchoDamageNumberActor::ReturnToPool()
{
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
	GetDamageNumberPools().FindOrAdd(GetWorld()).AddUnique(this);
}

void AReEchoDamageNumberActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;
	AddActorWorldOffset(FVector(0.0f, 0.0f, FloatSpeed * DeltaSeconds));

	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		// 正交相机的所有视线互相平行；统一使用相机前向，避免屏幕边缘文字产生透视式倾斜。
		SetActorRotation((-Camera->GetCameraRotation().Vector()).Rotation());
	}

	const float LifeProgress = DisplayDuration > 0.0f ? FMath::Clamp(ElapsedTime / DisplayDuration, 0.0f, 1.0f) : 1.0f;
	SetActorScale3D(FVector(FMath::Lerp(StartScale, EndScale, LifeProgress)));

	if (ElapsedTime >= DisplayDuration)
	{
		ReturnToPool();
	}
}
