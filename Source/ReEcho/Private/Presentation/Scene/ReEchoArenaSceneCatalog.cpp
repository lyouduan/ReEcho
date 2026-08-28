#include "Presentation/Scene/ReEchoArenaSceneCatalog.h"

#include "Presentation/Scene/ReEchoArenaSceneActor.h"

bool UReEchoArenaSceneCatalog::BuildRegistry(TMap<FName, TSubclassOf<AReEchoArenaSceneActor>>& OutRegistry,
                                             FString& OutError) const
{
	return BuildRegistry(Scenes, OutRegistry, OutError);
}

bool UReEchoArenaSceneCatalog::BuildRegistry(const TArray<FReEchoArenaSceneRegistration>& Registrations,
                                             TMap<FName, TSubclassOf<AReEchoArenaSceneActor>>& OutRegistry,
                                             FString& OutError)
{
	OutRegistry.Reset();
	OutError.Reset();
	for (const FReEchoArenaSceneRegistration& Registration : Registrations)
	{
		if (Registration.SceneId.IsNone() || !Registration.ArenaClass)
		{
			OutError = TEXT("Arena Scene registration requires a non-empty SceneId and ArenaClass.");
			OutRegistry.Reset();
			return false;
		}
		if (OutRegistry.Contains(Registration.SceneId))
		{
			OutError = FString::Printf(TEXT("Duplicate Arena Scene registration for SceneId=%s."),
			                           *Registration.SceneId.ToString());
			OutRegistry.Reset();
			return false;
		}
		OutRegistry.Add(Registration.SceneId, Registration.ArenaClass);
	}
	if (OutRegistry.IsEmpty())
	{
		OutError = TEXT("Arena Scene catalog is empty.");
		return false;
	}
	return true;
}
