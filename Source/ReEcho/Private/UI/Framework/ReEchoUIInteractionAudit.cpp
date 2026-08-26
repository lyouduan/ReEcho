#include "UI/Framework/ReEchoUIInteractionAudit.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "ReEcho.h"

namespace
{
FCriticalSection AuditWriteMutex;

FString Sanitize(FString Value)
{
	Value.ReplaceInline(TEXT("\r"), TEXT(" "));
	Value.ReplaceInline(TEXT("\n"), TEXT(" "));
	return Value;
}
} // namespace

void ReEchoUIInteractionAudit::Write(const FString& Event, const FString& Fields)
{
	const FString SafeEvent = Sanitize(Event);
	const FString SafeFields = Sanitize(Fields);
	const FString Message = FString::Printf(TEXT("[UIInteraction] event=%s %s"), *SafeEvent, *SafeFields);
	UE_LOG(LogReEcho, Display, TEXT("%s"), *Message);

	const FString LogDirectory = FPaths::ProjectLogDir();
	IFileManager::Get().MakeDirectory(*LogDirectory, true);
	const FString AuditLogPath = FPaths::Combine(LogDirectory, TEXT("UIInteractionAudit.log"));
	const FString TimestampedLine =
	    FString::Printf(TEXT("[%s] %s%s"), *FDateTime::UtcNow().ToIso8601(), *Message, LINE_TERMINATOR);
	FScopeLock Lock(&AuditWriteMutex);
	if (!FFileHelper::SaveStringToFile(TimestampedLine,
	                                   *AuditLogPath,
	                                   FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
	                                   &IFileManager::Get(),
	                                   FILEWRITE_Append))
	{
		UE_LOG(LogReEcho, Error, TEXT("[UIInteraction] Could not append audit file '%s'"), *AuditLogPath);
	}
}

FString ReEchoUIInteractionAudit::ScreenName(const EReEchoUIScreen Screen)
{
	switch (Screen)
	{
		case EReEchoUIScreen::Weather:
			return TEXT("Weather");
		case EReEchoUIScreen::EncounterHud:
			return TEXT("EncounterHud");
		case EReEchoUIScreen::PlayerHud:
			return TEXT("PlayerHud");
		case EReEchoUIScreen::StartMenu:
			return TEXT("StartMenu");
		case EReEchoUIScreen::Loadout:
			return TEXT("Loadout");
		case EReEchoUIScreen::Settings:
			return TEXT("Settings");
		case EReEchoUIScreen::About:
			return TEXT("About");
		case EReEchoUIScreen::Restart:
			return TEXT("Restart");
		case EReEchoUIScreen::EncounterTransition:
			return TEXT("EncounterTransition");
		case EReEchoUIScreen::TraitChoice:
			return TEXT("TraitChoice");
		case EReEchoUIScreen::InventoryShop:
			return TEXT("InventoryShop");
		case EReEchoUIScreen::Stats:
			return TEXT("Stats");
		default:
			return FString::Printf(TEXT("Unknown(%d)"), static_cast<int32>(Screen));
	}
}

FString ReEchoUIInteractionAudit::LayerName(const EReEchoUILayer Layer)
{
	switch (Layer)
	{
		case EReEchoUILayer::Weather:
			return TEXT("Weather");
		case EReEchoUILayer::GameplayHud:
			return TEXT("GameplayHud");
		case EReEchoUILayer::PlayerHud:
			return TEXT("PlayerHud");
		case EReEchoUILayer::BuildChoice:
			return TEXT("BuildChoice");
		case EReEchoUILayer::Transition:
			return TEXT("Transition");
		case EReEchoUILayer::Screen:
			return TEXT("Screen");
		case EReEchoUILayer::Pause:
			return TEXT("Pause");
		case EReEchoUILayer::Start:
			return TEXT("Start");
		case EReEchoUILayer::Loadout:
			return TEXT("Loadout");
		case EReEchoUILayer::Settings:
			return TEXT("Settings");
		default:
			return FString::Printf(TEXT("Unknown(%d)"), static_cast<int32>(Layer));
	}
}
