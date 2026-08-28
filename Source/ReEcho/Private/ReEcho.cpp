#include "ReEcho.h"

#include "Data/ReEchoCsvDataRegistry.h"
#include "HAL/PlatformProcess.h"
#include "Misc/OutputDeviceFile.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogReEcho);

class FReEchoModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		StartSessionLog();

		FReEchoCsvDataRegistry::RegisterBuiltInCsvBehaviors();
		const FReEchoCsvLoadResult LoadResult = FReEchoCsvDataRegistry::LoadAndPublishDefault();
		if (!LoadResult.bSuccess)
		{
			UE_LOG(LogReEcho, Fatal, TEXT("ReEcho Blueprint data failed to load:\n%s"), *LoadResult.FormatIssues());
		}
		UE_LOG(LogReEcho,
		       Log,
		       TEXT("ReEcho Blueprint data loaded from %s with %d runtime smoke rows, %d characters, %d cards, %d elements "
		            "and %d reactions"),
		       *FReEchoCsvDataRegistry::GetDefaultDataCatalogPath(),
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->RuntimeSmokeRows.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Characters.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Cards.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Elements.Num() : 0,
		       LoadResult.Snapshot.IsValid() ? LoadResult.Snapshot->Reactions.Num() : 0);
	}

	virtual void ShutdownModule() override
	{
		StopSessionLog();
		FDefaultGameModuleImpl::ShutdownModule();
	}

private:
	void StartSessionLog()
	{
#if !UE_BUILD_SHIPPING
		const FString StartedAt = FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
		const FString FileName =
		    FString::Printf(TEXT("ReEcho-session-%s-pid%u.log"), *StartedAt, FPlatformProcess::GetCurrentProcessId());
		const FString SessionLogPath = FPaths::Combine(FPaths::ProjectLogDir(), FileName);
		SessionLog = MakeUnique<FOutputDeviceFile>(*SessionLogPath,
		                                           /*bDisableBackup=*/true,
		                                           /*bAppendIfExists=*/false,
		                                           /*bCreateWriterLazily=*/false);
		if (GLog)
		{
			GLog->AddOutputDevice(SessionLog.Get());
			UE_LOG(LogReEcho, Log, TEXT("[SessionLog] started local=%s file=%s"), *StartedAt, *SessionLogPath);
		}
#endif
	}

	void StopSessionLog()
	{
#if !UE_BUILD_SHIPPING
		if (!SessionLog)
		{
			return;
		}
		UE_LOG(LogReEcho, Log, TEXT("[SessionLog] stopping file=%s"), SessionLog->GetFilename());
		if (GLog)
		{
			GLog->FlushThreadedLogs();
			GLog->RemoveOutputDevice(SessionLog.Get());
		}
		SessionLog->TearDown();
		SessionLog.Reset();
#endif
	}

#if !UE_BUILD_SHIPPING
	TUniquePtr<FOutputDeviceFile> SessionLog;
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE(FReEchoModule, ReEcho, "ReEcho");
