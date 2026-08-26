#include "ReEchoAudioCatalog.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Misc/FileHelper.h"

namespace ReEchoAudioCatalogDetail
{
static bool ParseCsv(const FString& Content, TArray<TArray<FString>>& OutRows, FString& OutError)
{
	TArray<FString> Row;
	FString Cell;
	bool bQuoted = false;
	for (int32 Index = 0; Index < Content.Len(); ++Index)
	{
		const TCHAR Ch = Content[Index];
		if (bQuoted)
		{
			if (Ch == TEXT('"'))
			{
				if (Index + 1 < Content.Len() && Content[Index + 1] == TEXT('"'))
				{
					Cell.AppendChar(TEXT('"'));
					++Index;
				}
				else
				{
					bQuoted = false;
				}
			}
			else
			{
				Cell.AppendChar(Ch);
			}
			continue;
		}
		if (Ch == TEXT('"') && Cell.IsEmpty())
		{
			bQuoted = true;
		}
		else if (Ch == TEXT(','))
		{
			Row.Add(MoveTemp(Cell));
			Cell.Reset();
		}
		else if (Ch == TEXT('\n'))
		{
			if (Cell.EndsWith(TEXT("\r")))
			{
				Cell.LeftChopInline(1);
			}
			Row.Add(MoveTemp(Cell));
			Cell.Reset();
			OutRows.Add(MoveTemp(Row));
			Row.Reset();
		}
		else
		{
			Cell.AppendChar(Ch);
		}
	}
	if (bQuoted)
	{
		OutError = TEXT("unterminated quoted field");
		return false;
	}
	if (!Cell.IsEmpty() || !Row.IsEmpty())
	{
		if (Cell.EndsWith(TEXT("\r")))
		{
			Cell.LeftChopInline(1);
		}
		Row.Add(MoveTemp(Cell));
		OutRows.Add(MoveTemp(Row));
	}
	return true;
}

static bool ParseBus(const FString& Value, EReEchoAudioBus& Out)
{
	if (Value == TEXT("Music"))
	{
		Out = EReEchoAudioBus::Music;
		return true;
	}
	if (Value == TEXT("Ambience"))
	{
		Out = EReEchoAudioBus::Ambience;
		return true;
	}
	if (Value == TEXT("CombatSfx"))
	{
		Out = EReEchoAudioBus::CombatSfx;
		return true;
	}
	if (Value == TEXT("UiSfx"))
	{
		Out = EReEchoAudioBus::UiSfx;
		return true;
	}
	return false;
}

static bool ParseType(const FString& Value, EReEchoAudioEventType& Out)
{
	if (Value == TEXT("OneShot"))
	{
		Out = EReEchoAudioEventType::OneShot;
		return true;
	}
	if (Value == TEXT("Loop"))
	{
		Out = EReEchoAudioEventType::Loop;
		return true;
	}
	return false;
}

static bool ParsePausePolicy(const FString& Value, EReEchoAudioPausePolicy& Out)
{
	if (Value == TEXT("PauseWithGame"))
	{
		Out = EReEchoAudioPausePolicy::PauseWithGame;
		return true;
	}
	if (Value == TEXT("ContinueOnPause"))
	{
		Out = EReEchoAudioPausePolicy::ContinueOnPause;
		return true;
	}
	return false;
}

static bool ParseBool(const FString& Value, bool& Out)
{
	if (Value == TEXT("true"))
	{
		Out = true;
		return true;
	}
	if (Value == TEXT("false"))
	{
		Out = false;
		return true;
	}
	return false;
}

static bool ParseFloat(const FString& Value, float& Out)
{
	if (Value.IsEmpty() || !Value.IsNumeric())
	{
		return false;
	}
	Out = FCString::Atof(*Value);
	return FMath::IsFinite(Out);
}

static bool ParseInt(const FString& Value, int32& Out)
{
	if (Value.IsEmpty() || !Value.IsNumeric())
	{
		return false;
	}
	const double Parsed = FCString::Atod(*Value);
	if (!FMath::IsFinite(Parsed) || Parsed != FMath::RoundToDouble(Parsed))
	{
		return false;
	}
	Out = static_cast<int32>(Parsed);
	return true;
}
}

void FReEchoAudioCatalog::AddDefinition(const FReEchoAudioEventDefinition& Definition)
{
	Definitions.Add({Definition.EventId, Definition.VariantId}, Definition);
}

void FReEchoAudioCatalog::Clear()
{
	CancelPreload();
	Definitions.Empty();
	SoftAssetPaths.Reset();
	LastLoadError.Reset();
}

const FReEchoAudioEventDefinition* FReEchoAudioCatalog::FindDefinition(const FName EventId, const FName VariantId) const
{
	if (!VariantId.IsNone())
	{
		if (const FReEchoAudioEventDefinition* Exact = Definitions.Find({EventId, VariantId}))
		{
			return Exact;
		}
	}
	return Definitions.Find({EventId, NAME_None});
}

bool FReEchoAudioCatalog::FailLoad(const FString& Message)
{
	LastLoadError = Message;
	UE_LOG(LogReEchoAudio, Error, TEXT("FReEchoAudioCatalog::LoadCatalog: %s"), *Message);
	return false;
}

bool FReEchoAudioCatalog::LoadCatalog(const FString& CsvPath)
{
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *CsvPath))
	{
		return FailLoad(FString::Printf(TEXT("cannot read %s; preserving previous catalog"), *CsvPath));
	}

	TArray<TArray<FString>> Rows;
	FString ParseError;
	if (!ReEchoAudioCatalogDetail::ParseCsv(FileContent, Rows, ParseError))
	{
		return FailLoad(FString::Printf(TEXT("%s: %s"), *CsvPath, *ParseError));
	}
	if (Rows.Num() < 2)
	{
		return FailLoad(FString::Printf(TEXT("%s has no data rows"), *CsvPath));
	}

	static const TArray<FString> RequiredHeaders = {TEXT("EventId"),
	                                                TEXT("VariantId"),
	                                                TEXT("AssetPath"),
	                                                TEXT("Bus"),
	                                                TEXT("EventType"),
	                                                TEXT("Spatial3D"),
	                                                TEXT("BaseVolume"),
	                                                TEXT("PitchMin"),
	                                                TEXT("PitchMax"),
	                                                TEXT("CooldownSeconds"),
	                                                TEXT("MaxConcurrency"),
	                                                TEXT("Priority"),
	                                                TEXT("PausePolicy"),
	                                                TEXT("AttenuationMin"),
	                                                TEXT("AttenuationMax")};

	TMap<FString, int32> HeaderIndices;
	for (int32 Column = 0; Column < Rows[0].Num(); ++Column)
	{
		HeaderIndices.Add(Rows[0][Column].TrimStartAndEnd(), Column);
	}
	for (const FString& Header : RequiredHeaders)
	{
		if (!HeaderIndices.Contains(Header))
		{
			return FailLoad(FString::Printf(TEXT("%s is missing required column %s"), *CsvPath, *Header));
		}
	}
	if (HeaderIndices.Num() != RequiredHeaders.Num())
	{
		return FailLoad(FString::Printf(TEXT("%s header does not match the locked 15-column schema"), *CsvPath));
	}

	TMap<FReEchoAudioCatalogKey, FReEchoAudioEventDefinition> PendingDefinitions;
	TArray<FSoftObjectPath> PendingAssetPaths;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<FString>& Row = Rows[RowIndex];
		if (Row.Num() == 0)
		{
			continue;
		}
		const int32 CsvLine = RowIndex + 1;
		auto Cell = [&](const TCHAR* Name) -> FString
		{
			const int32* Index = HeaderIndices.Find(Name);
			return Index && Row.IsValidIndex(*Index) ? Row[*Index].TrimStartAndEnd() : FString();
		};
		auto RowError = [&](const TCHAR* Column, const FString& Reason) -> bool
		{
			return FailLoad(
			    FString::Printf(TEXT("%s:%d:%s: %s; preserving previous catalog"), *CsvPath, CsvLine, Column, *Reason));
		};

		FReEchoAudioEventDefinition Def;
		const FString EventId = Cell(TEXT("EventId"));
		if (EventId.IsEmpty())
		{
			return RowError(TEXT("EventId"), TEXT("required value is empty"));
		}
		Def.EventId = FName(*EventId);
		Def.VariantId = FName(*Cell(TEXT("VariantId")));
		const FReEchoAudioCatalogKey Key{Def.EventId, Def.VariantId};
		if (PendingDefinitions.Contains(Key))
		{
			return RowError(TEXT("VariantId"), TEXT("duplicate event/variant pair"));
		}

		const FString AssetPath = Cell(TEXT("AssetPath"));
		if (!AssetPath.IsEmpty())
		{
			const FSoftObjectPath Path(AssetPath);
			if (!Path.IsValid())
			{
				return RowError(TEXT("AssetPath"), TEXT("invalid Unreal soft object path"));
			}
			Def.Sound = TSoftObjectPtr<USoundBase>(Path);
			PendingAssetPaths.AddUnique(Path);
		}
		if (!ReEchoAudioCatalogDetail::ParseBus(Cell(TEXT("Bus")), Def.Bus))
		{
			return RowError(TEXT("Bus"), TEXT("unsupported bus"));
		}
		if (!ReEchoAudioCatalogDetail::ParseType(Cell(TEXT("EventType")), Def.Type))
		{
			return RowError(TEXT("EventType"), TEXT("must be OneShot or Loop"));
		}
		if (!ReEchoAudioCatalogDetail::ParseBool(Cell(TEXT("Spatial3D")), Def.bSpatial3D))
		{
			return RowError(TEXT("Spatial3D"), TEXT("must be lowercase true or false"));
		}
		if (!ReEchoAudioCatalogDetail::ParsePausePolicy(Cell(TEXT("PausePolicy")), Def.PausePolicy))
		{
			return RowError(TEXT("PausePolicy"), TEXT("unsupported pause policy"));
		}
		if (!ReEchoAudioCatalogDetail::ParseFloat(Cell(TEXT("BaseVolume")), Def.BaseVolume) || Def.BaseVolume < 0.0f ||
		    Def.BaseVolume > 1.0f)
		{
			return RowError(TEXT("BaseVolume"), TEXT("must be in [0,1]"));
		}
		if (!ReEchoAudioCatalogDetail::ParseFloat(Cell(TEXT("PitchMin")), Def.PitchMin) || Def.PitchMin <= 0.0f)
		{
			return RowError(TEXT("PitchMin"), TEXT("must be > 0"));
		}
		if (!ReEchoAudioCatalogDetail::ParseFloat(Cell(TEXT("PitchMax")), Def.PitchMax) || Def.PitchMax < Def.PitchMin)
		{
			return RowError(TEXT("PitchMax"), TEXT("must be >= PitchMin"));
		}
		if (!ReEchoAudioCatalogDetail::ParseFloat(Cell(TEXT("CooldownSeconds")), Def.CooldownSeconds) ||
		    Def.CooldownSeconds < 0.0f)
		{
			return RowError(TEXT("CooldownSeconds"), TEXT("must be >= 0"));
		}
		if (!ReEchoAudioCatalogDetail::ParseInt(Cell(TEXT("MaxConcurrency")), Def.MaxConcurrency) ||
		    Def.MaxConcurrency < 0)
		{
			return RowError(TEXT("MaxConcurrency"), TEXT("must be an integer >= 0"));
		}
		if (!ReEchoAudioCatalogDetail::ParseInt(Cell(TEXT("Priority")), Def.Priority) || Def.Priority < 0)
		{
			return RowError(TEXT("Priority"), TEXT("must be an integer >= 0"));
		}
		if (!ReEchoAudioCatalogDetail::ParseFloat(Cell(TEXT("AttenuationMin")), Def.AttenuationMin) ||
		    Def.AttenuationMin < 0.0f)
		{
			return RowError(TEXT("AttenuationMin"), TEXT("must be >= 0"));
		}
		if (!ReEchoAudioCatalogDetail::ParseFloat(Cell(TEXT("AttenuationMax")), Def.AttenuationMax) ||
		    Def.AttenuationMax < Def.AttenuationMin)
		{
			return RowError(TEXT("AttenuationMax"), TEXT("must be >= AttenuationMin"));
		}

		PendingDefinitions.Add(Key, Def);
	}

	if (PendingDefinitions.IsEmpty())
	{
		return FailLoad(FString::Printf(TEXT("%s contains no valid definitions"), *CsvPath));
	}
	CancelPreload();
	Definitions = MoveTemp(PendingDefinitions);
	SoftAssetPaths = MoveTemp(PendingAssetPaths);
	LastLoadError.Reset();
	UE_LOG(LogReEchoAudio, Log, TEXT("Loaded %d audio definitions atomically from %s"), Definitions.Num(), *CsvPath);
	return true;
}

void FReEchoAudioCatalog::PreloadSoftAssets(FStreamableManager& StreamableManager)
{
	CancelPreload();
	if (SoftAssetPaths.IsEmpty())
	{
		PreloadState = EReEchoAudioCatalogPreloadState::Ready;
		return;
	}
	PreloadState = EReEchoAudioCatalogPreloadState::Loading;
	PreloadHandle = StreamableManager.RequestAsyncLoad(
	    SoftAssetPaths,
	    FStreamableDelegate::CreateRaw(this, &FReEchoAudioCatalog::HandlePreloadComplete),
	    FStreamableManager::AsyncLoadHighPriority);
	if (!PreloadHandle.IsValid())
	{
		PreloadState = EReEchoAudioCatalogPreloadState::Failed;
	}
}

void FReEchoAudioCatalog::HandlePreloadComplete()
{
	for (const FSoftObjectPath& Path : SoftAssetPaths)
	{
		if (Path.ResolveObject() == nullptr)
		{
			PreloadState = EReEchoAudioCatalogPreloadState::Failed;
			UE_LOG(LogReEchoAudio,
			       Warning,
			       TEXT("Audio preload incomplete; missing asset %s (retry remains available)."),
			       *Path.ToString());
			return;
		}
	}
	PreloadState = EReEchoAudioCatalogPreloadState::Ready;
}

void FReEchoAudioCatalog::CancelPreload()
{
	if (PreloadHandle.IsValid() && !PreloadHandle->HasLoadCompleted())
	{
		PreloadHandle->CancelHandle();
	}
	PreloadHandle.Reset();
	PreloadState = EReEchoAudioCatalogPreloadState::NotStarted;
}
