#include "Presentation/Enemy/ReEchoEnemyGameplayClassRegistry.h"

#include "Graybox/ReEchoEnemyActor.h"

TSubclassOf<AReEchoEnemyActor> UReEchoEnemyGameplayClassRegistry::ResolveGameplayClass(const FName PresentationId) const
{
	for (const FReEchoEnemyGameplayClassEntry& Entry : Entries)
	{
		if (Entry.PresentationId == PresentationId)
		{
			return Entry.GameplayClass;
		}
	}
	return nullptr;
}
