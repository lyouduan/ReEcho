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

		const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
		if (!LoadResult.bSuccess)
		{
			UE_LOG(LogReEcho, Fatal, TEXT("ReEcho CSV data failed to load:\n%s"), *LoadResult.FormatIssues());
		}
		UE_LOG(LogReEcho,
		       Log,
		       TEXT("ReEcho CSV data loaded from %s with %d runtime smoke rows"),
		       *FReEchoCsvDataRegistry::GetDefaultDataDirectory(),
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->RuntimeSmokeRows.Num() : 0);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FReEchoModule, ReEcho, "ReEcho");
