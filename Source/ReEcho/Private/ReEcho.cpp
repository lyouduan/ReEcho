#include "ReEcho.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogReEcho);

class FReEchoModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		FReEchoCsvDataRegistry::RegisterBuiltInCsvBehaviors();
		const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
		if (!LoadResult.bSuccess)
		{
			UE_LOG(LogReEcho, Fatal, TEXT("ReEcho CSV data failed to load:\n%s"), *LoadResult.FormatIssues());
		}
		UE_LOG(LogReEcho,
		       Log,
		       TEXT("ReEcho CSV data loaded from %s with %d runtime smoke rows, %d characters, %d cards, %d elements "
		            "and %d reactions"),
		       *FReEchoCsvDataRegistry::GetDefaultDataDirectory(),
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->RuntimeSmokeRows.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Characters.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Cards.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Elements.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Reactions.Num() : 0);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FReEchoModule, ReEcho, "ReEcho");
