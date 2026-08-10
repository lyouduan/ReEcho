#include "Data/ReEchoCsvDataRegistry.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "ReEcho.h"

namespace
{
constexpr const TCHAR* ManifestFileName = TEXT("reecho_data_manifest.csv");
constexpr const TCHAR* RuntimeSmokeTableId = TEXT("RuntimeSmoke");
constexpr const TCHAR* RuntimeSmokeEffectsTableId = TEXT("RuntimeSmokeEffects");
constexpr const TCHAR* DefaultBehaviorId = TEXT("RuntimeSmoke.LogValue");
constexpr const TCHAR* DefaultEffectKind = TEXT("ScalarModifier");

struct FReEchoCsvRow
{
	int32 Line = 0;
	TMap<FString, FString> Cells;
};

struct FReEchoCsvTable
{
	FString File;
	TArray<FString> Headers;
	TArray<FReEchoCsvRow> Rows;
};

struct FReEchoManifestEntry
{
	FString TableId;
	FString FileName;
	FString PrimaryKey;
	int32 SchemaVersion = 0;
	int32 Line = 0;
};

FCriticalSection RegistryCriticalSection;
TSharedPtr<const FReEchoCsvDataSnapshot> PublishedSnapshot;
TSet<FName> RegisteredBehaviorIds;
TSet<FName> RegisteredEffectKinds;
bool bDefaultRegistrationsReady = false;

FString NormalizeFileForIssue(const FString& Path)
{
	FString Normalized = Path;
	FPaths::NormalizeFilename(Normalized);
	const FString ContentData = TEXT("Content/Data/");
	const int32 Index = Normalized.Find(ContentData, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (Index != INDEX_NONE)
	{
		return Normalized.Mid(Index);
	}
	return FPaths::GetCleanFilename(Normalized);
}

void AddIssue(TArray<FReEchoCsvIssue>& Issues,
              const FString& File,
              const int32 Line,
              const FString& Field,
              const FString& Message)
{
	FReEchoCsvIssue Issue;
	Issue.File = NormalizeFileForIssue(File);
	Issue.Line = Line;
	Issue.Field = Field;
	Issue.Message = Message;
	Issues.Add(Issue);
}

FString TrimCell(const FString& Value)
{
	return Value.TrimStartAndEnd();
}

bool IsStableId(const FString& Value)
{
	if (Value.IsEmpty() || Value != TrimCell(Value))
	{
		return false;
	}

	for (const TCHAR Character : Value)
	{
		if (FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-') || Character == TEXT('.'))
		{
			continue;
		}
		return false;
	}
	return true;
}

bool ParseCsvFile(const FString& FilePath, FReEchoCsvTable& OutTable, TArray<FReEchoCsvIssue>& Issues)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FilePath))
	{
		AddIssue(Issues, FilePath, 0, TEXT("File"), TEXT("File could not be read"));
		return false;
	}
	if (Bytes.Num() >= 3 && Bytes[0] == 0xEF && Bytes[1] == 0xBB && Bytes[2] == 0xBF)
	{
		AddIssue(Issues, FilePath, 1, TEXT("File"), TEXT("CSV must be UTF-8 without BOM"));
		return false;
	}

	const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
	const FString Text(Converted.Length(), Converted.Get());
	OutTable.File = FilePath;

	TArray<TArray<FString>> RawRows;
	TArray<int32> RowLines;
	TArray<FString> CurrentRow;
	FString CurrentField;
	bool bInQuotes = false;
	int32 Line = 1;
	int32 RowStartLine = 1;

	for (int32 Index = 0; Index < Text.Len(); ++Index)
	{
		const TCHAR Character = Text[Index];
		if (bInQuotes)
		{
			if (Character == TEXT('"'))
			{
				if (Index + 1 < Text.Len() && Text[Index + 1] == TEXT('"'))
				{
					CurrentField.AppendChar(TEXT('"'));
					++Index;
				}
				else
				{
					bInQuotes = false;
				}
			}
			else
			{
				if (Character == TEXT('\n'))
				{
					++Line;
				}
				CurrentField.AppendChar(Character);
			}
			continue;
		}

		if (Character == TEXT('"'))
		{
			if (CurrentField.IsEmpty())
			{
				bInQuotes = true;
			}
			else
			{
				AddIssue(Issues, FilePath, Line, TEXT("CSV"), TEXT("Quote must start a quoted cell"));
				return false;
			}
		}
		else if (Character == TEXT(','))
		{
			CurrentRow.Add(CurrentField);
			CurrentField.Reset();
		}
		else if (Character == TEXT('\r') || Character == TEXT('\n'))
		{
			CurrentRow.Add(CurrentField);
			CurrentField.Reset();
			RawRows.Add(CurrentRow);
			RowLines.Add(RowStartLine);
			CurrentRow.Reset();
			if (Character == TEXT('\r') && Index + 1 < Text.Len() && Text[Index + 1] == TEXT('\n'))
			{
				++Index;
			}
			++Line;
			RowStartLine = Line;
		}
		else
		{
			CurrentField.AppendChar(Character);
		}
	}

	if (bInQuotes)
	{
		AddIssue(Issues, FilePath, Line, TEXT("CSV"), TEXT("Quoted cell was not closed"));
		return false;
	}
	if (!CurrentField.IsEmpty() || CurrentRow.Num() > 0)
	{
		CurrentRow.Add(CurrentField);
		RawRows.Add(CurrentRow);
		RowLines.Add(RowStartLine);
	}
	if (RawRows.Num() == 0)
	{
		AddIssue(Issues, FilePath, 1, TEXT("CSV"), TEXT("CSV file is empty"));
		return false;
	}

	OutTable.Headers = RawRows[0];
	for (FString& Header : OutTable.Headers)
	{
		Header = TrimCell(Header);
	}

	TSet<FString> UniqueHeaders;
	for (const FString& Header : OutTable.Headers)
	{
		if (Header.IsEmpty() || UniqueHeaders.Contains(Header))
		{
			AddIssue(Issues, FilePath, RowLines[0], Header, TEXT("Header is empty or duplicated"));
			return false;
		}
		UniqueHeaders.Add(Header);
	}

	for (int32 RawIndex = 1; RawIndex < RawRows.Num(); ++RawIndex)
	{
		const TArray<FString>& RawRow = RawRows[RawIndex];
		if (RawRow.Num() == 1 && TrimCell(RawRow[0]).IsEmpty())
		{
			continue;
		}
		if (RawRow.Num() != OutTable.Headers.Num())
		{
			AddIssue(Issues,
			         FilePath,
			         RowLines[RawIndex],
			         TEXT("CSV"),
			         FString::Printf(TEXT("Expected %d cells, got %d"), OutTable.Headers.Num(), RawRow.Num()));
			continue;
		}

		FReEchoCsvRow Row;
		Row.Line = RowLines[RawIndex];
		for (int32 ColumnIndex = 0; ColumnIndex < OutTable.Headers.Num(); ++ColumnIndex)
		{
			const FString Cell = TrimCell(RawRow[ColumnIndex]);
			if (Cell.StartsWith(TEXT("=")) || Cell.StartsWith(TEXT("@")))
			{
				AddIssue(Issues,
				         FilePath,
				         RowLines[RawIndex],
				         OutTable.Headers[ColumnIndex],
				         TEXT("CSV cells must not contain spreadsheet formulas"));
			}
			Row.Cells.Add(OutTable.Headers[ColumnIndex], Cell);
		}
		OutTable.Rows.Add(Row);
	}
	return Issues.Num() == 0;
}

bool HasExactColumns(const FReEchoCsvTable& Table,
                     const TArray<FString>& ExpectedColumns,
                     TArray<FReEchoCsvIssue>& Issues)
{
	TSet<FString> ExpectedSet;
	for (const FString& ExpectedColumn : ExpectedColumns)
	{
		ExpectedSet.Add(ExpectedColumn);
	}
	bool bValid = true;
	for (const FString& ExpectedColumn : ExpectedColumns)
	{
		if (!Table.Headers.Contains(ExpectedColumn))
		{
			AddIssue(Issues, Table.File, 1, ExpectedColumn, TEXT("Required column is missing"));
			bValid = false;
		}
	}
	for (const FString& Header : Table.Headers)
	{
		if (!ExpectedSet.Contains(Header))
		{
			AddIssue(Issues, Table.File, 1, Header, TEXT("Unsupported column"));
			bValid = false;
		}
	}
	return bValid;
}

bool RequireCell(const FReEchoCsvTable& Table,
                 const FReEchoCsvRow& Row,
                 const FString& Field,
                 FString& OutValue,
                 TArray<FReEchoCsvIssue>& Issues)
{
	const FString* Found = Row.Cells.Find(Field);
	OutValue = Found ? *Found : FString();
	if (OutValue.IsEmpty())
	{
		AddIssue(Issues, Table.File, Row.Line, Field, TEXT("Required value is empty"));
		return false;
	}
	return true;
}

bool RequireStableId(const FReEchoCsvTable& Table,
                     const FReEchoCsvRow& Row,
                     const FString& Field,
                     FName& OutValue,
                     TArray<FReEchoCsvIssue>& Issues)
{
	FString Text;
	if (!RequireCell(Table, Row, Field, Text, Issues))
	{
		return false;
	}
	if (!IsStableId(Text))
	{
		AddIssue(Issues,
		         Table.File,
		         Row.Line,
		         Field,
		         TEXT("Stable id may only contain letters, digits, '_', '-' or '.' and no outer whitespace"));
		return false;
	}
	OutValue = FName(*Text);
	return true;
}

bool RequireBool(const FReEchoCsvTable& Table,
                 const FReEchoCsvRow& Row,
                 const FString& Field,
                 bool& OutValue,
                 TArray<FReEchoCsvIssue>& Issues)
{
	FString Text;
	if (!RequireCell(Table, Row, Field, Text, Issues))
	{
		return false;
	}
	if (Text == TEXT("true"))
	{
		OutValue = true;
		return true;
	}
	if (Text == TEXT("false"))
	{
		OutValue = false;
		return true;
	}
	AddIssue(Issues, Table.File, Row.Line, Field, TEXT("Boolean must be lowercase true or false"));
	return false;
}

bool RequireFloat(const FReEchoCsvTable& Table,
                  const FReEchoCsvRow& Row,
                  const FString& Field,
                  const float MinValue,
                  const float MaxValue,
                  float& OutValue,
                  TArray<FReEchoCsvIssue>& Issues)
{
	FString Text;
	if (!RequireCell(Table, Row, Field, Text, Issues))
	{
		return false;
	}
	if (!LexTryParseString(OutValue, *Text) || !FMath::IsFinite(OutValue))
	{
		AddIssue(Issues, Table.File, Row.Line, Field, TEXT("Value must be a finite number"));
		return false;
	}
	if (OutValue < MinValue || OutValue > MaxValue)
	{
		AddIssue(
		    Issues,
		    Table.File,
		    Row.Line,
		    Field,
		    FString::Printf(TEXT("Value %.4f is outside supported range %.4f..%.4f"), OutValue, MinValue, MaxValue));
		return false;
	}
	return true;
}

bool RequireInt(const FReEchoCsvTable& Table,
                const FReEchoCsvRow& Row,
                const FString& Field,
                int32& OutValue,
                TArray<FReEchoCsvIssue>& Issues)
{
	float FloatValue = 0.0f;
	if (!RequireFloat(Table, Row, Field, 0.0f, 1000000.0f, FloatValue, Issues))
	{
		return false;
	}
	if (!FMath::IsNearlyEqual(FloatValue, FMath::RoundToFloat(FloatValue)))
	{
		AddIssue(Issues, Table.File, Row.Line, Field, TEXT("Value must be an integer"));
		return false;
	}
	OutValue = static_cast<int32>(FloatValue);
	return true;
}

bool RequireValueOp(const FReEchoCsvTable& Table,
                    const FReEchoCsvRow& Row,
                    const FString& Field,
                    EReEchoCsvValueOp& OutValue,
                    TArray<FReEchoCsvIssue>& Issues)
{
	FString Text;
	if (!RequireCell(Table, Row, Field, Text, Issues))
	{
		return false;
	}
	if (Text == TEXT("Add"))
	{
		OutValue = EReEchoCsvValueOp::Add;
		return true;
	}
	if (Text == TEXT("Multiply"))
	{
		OutValue = EReEchoCsvValueOp::Multiply;
		return true;
	}
	if (Text == TEXT("Override"))
	{
		OutValue = EReEchoCsvValueOp::Override;
		return true;
	}
	AddIssue(Issues, Table.File, Row.Line, Field, TEXT("Value operation must be Add, Multiply or Override"));
	return false;
}

bool ReadManifest(const FString& DataDirectory,
                  TMap<FString, FReEchoManifestEntry>& OutEntries,
                  TArray<FReEchoCsvIssue>& Issues)
{
	FReEchoCsvTable Manifest;
	const FString ManifestPath = FPaths::Combine(DataDirectory, ManifestFileName);
	if (!ParseCsvFile(ManifestPath, Manifest, Issues))
	{
		return false;
	}

	HasExactColumns(Manifest, {TEXT("SchemaVersion"), TEXT("TableId"), TEXT("FileName"), TEXT("PrimaryKey")}, Issues);
	TSet<FString> SeenTables;
	for (const FReEchoCsvRow& Row : Manifest.Rows)
	{
		FReEchoManifestEntry Entry;
		Entry.Line = Row.Line;
		RequireInt(Manifest, Row, TEXT("SchemaVersion"), Entry.SchemaVersion, Issues);
		RequireCell(Manifest, Row, TEXT("TableId"), Entry.TableId, Issues);
		RequireCell(Manifest, Row, TEXT("FileName"), Entry.FileName, Issues);
		RequireCell(Manifest, Row, TEXT("PrimaryKey"), Entry.PrimaryKey, Issues);
		if (Entry.SchemaVersion != FReEchoCsvDataRegistry::SupportedSchemaVersion)
		{
			AddIssue(Issues,
			         Manifest.File,
			         Row.Line,
			         TEXT("SchemaVersion"),
			         FString::Printf(TEXT("Unsupported schema version %d"), Entry.SchemaVersion));
		}
		if (Entry.TableId.IsEmpty() || SeenTables.Contains(Entry.TableId))
		{
			AddIssue(Issues, Manifest.File, Row.Line, TEXT("TableId"), TEXT("TableId is empty or duplicated"));
		}
		SeenTables.Add(Entry.TableId);
		OutEntries.Add(Entry.TableId, Entry);
	}

	for (const FString RequiredTable : {FString(RuntimeSmokeTableId), FString(RuntimeSmokeEffectsTableId)})
	{
		if (!OutEntries.Contains(RequiredTable))
		{
			AddIssue(Issues,
			         Manifest.File,
			         1,
			         TEXT("TableId"),
			         FString::Printf(TEXT("Manifest missing table %s"), *RequiredTable));
		}
	}
	return Issues.Num() == 0;
}

bool ReadRuntimeSmokeTable(const FString& DataDirectory,
                           const FReEchoManifestEntry& Entry,
                           FReEchoCsvDataSnapshot& Snapshot,
                           TArray<FReEchoCsvIssue>& Issues)
{
	FReEchoCsvTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	HasExactColumns(Table,
	                {TEXT("Id"),
	                 TEXT("DisplayNameKey"),
	                 TEXT("Enabled"),
	                 TEXT("TestScalar"),
	                 TEXT("TestPercent"),
	                 TEXT("DistanceCm"),
	                 TEXT("DurationSeconds"),
	                 TEXT("BehaviorId"),
	                 TEXT("EffectKind"),
	                 TEXT("ModifierOp")},
	                Issues);

	TSet<FName> SeenIds;
	for (const FReEchoCsvRow& Row : Table.Rows)
	{
		FReEchoRuntimeSmokeRow RuntimeRow;
		RequireStableId(Table, Row, TEXT("Id"), RuntimeRow.Id, Issues);
		RequireCell(Table, Row, TEXT("DisplayNameKey"), RuntimeRow.DisplayNameKey, Issues);
		RequireBool(Table, Row, TEXT("Enabled"), RuntimeRow.bEnabled, Issues);
		RequireFloat(Table, Row, TEXT("TestScalar"), 0.0f, 100000.0f, RuntimeRow.TestScalar, Issues);
		RequireFloat(Table, Row, TEXT("TestPercent"), 0.0f, 1.0f, RuntimeRow.TestPercent, Issues);
		RequireFloat(Table, Row, TEXT("DistanceCm"), 0.0f, 1000000.0f, RuntimeRow.DistanceCm, Issues);
		RequireFloat(Table, Row, TEXT("DurationSeconds"), 0.0f, 3600.0f, RuntimeRow.DurationSeconds, Issues);
		RequireStableId(Table, Row, TEXT("BehaviorId"), RuntimeRow.BehaviorId, Issues);
		RequireStableId(Table, Row, TEXT("EffectKind"), RuntimeRow.EffectKind, Issues);
		RequireValueOp(Table, Row, TEXT("ModifierOp"), RuntimeRow.ModifierOp, Issues);

		if (RuntimeRow.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(RuntimeRow.Id))
		{
			AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (!FReEchoCsvDataRegistry::IsBehaviorIdRegistered(RuntimeRow.BehaviorId))
		{
			AddIssue(Issues, Table.File, Row.Line, TEXT("BehaviorId"), TEXT("Unknown registered C++ behavior id"));
		}
		if (!FReEchoCsvDataRegistry::IsEffectKindRegistered(RuntimeRow.EffectKind))
		{
			AddIssue(Issues, Table.File, Row.Line, TEXT("EffectKind"), TEXT("Unknown registered C++ effect kind"));
		}
		SeenIds.Add(RuntimeRow.Id);
		Snapshot.RuntimeSmokeRows.Add(RuntimeRow.Id, RuntimeRow);
	}
	return Issues.Num() == 0;
}

bool ReadRuntimeSmokeEffectsTable(const FString& DataDirectory,
                                  const FReEchoManifestEntry& Entry,
                                  FReEchoCsvDataSnapshot& Snapshot,
                                  TArray<FReEchoCsvIssue>& Issues)
{
	FReEchoCsvTable Table;
	const FString TablePath = FPaths::Combine(DataDirectory, Entry.FileName);
	if (!ParseCsvFile(TablePath, Table, Issues))
	{
		return false;
	}

	HasExactColumns(
	    Table, {TEXT("Id"), TEXT("RuntimeRowId"), TEXT("ParamName"), TEXT("ValueOp"), TEXT("Value")}, Issues);

	TSet<FName> SeenIds;
	for (const FReEchoCsvRow& Row : Table.Rows)
	{
		FReEchoRuntimeSmokeEffectRow EffectRow;
		RequireStableId(Table, Row, TEXT("Id"), EffectRow.Id, Issues);
		RequireStableId(Table, Row, TEXT("RuntimeRowId"), EffectRow.RuntimeRowId, Issues);
		RequireStableId(Table, Row, TEXT("ParamName"), EffectRow.ParamName, Issues);
		RequireValueOp(Table, Row, TEXT("ValueOp"), EffectRow.ValueOp, Issues);
		RequireFloat(Table, Row, TEXT("Value"), -100000.0f, 100000.0f, EffectRow.Value, Issues);

		if (EffectRow.Id.IsNone())
		{
			continue;
		}
		if (SeenIds.Contains(EffectRow.Id))
		{
			AddIssue(Issues, Table.File, Row.Line, TEXT("Id"), TEXT("Duplicate id"));
		}
		if (FReEchoRuntimeSmokeRow* Parent = Snapshot.RuntimeSmokeRows.Find(EffectRow.RuntimeRowId))
		{
			Parent->Effects.Add(EffectRow);
		}
		else
		{
			AddIssue(Issues, Table.File, Row.Line, TEXT("RuntimeRowId"), TEXT("Unknown RuntimeSmoke.Id reference"));
		}
		SeenIds.Add(EffectRow.Id);
	}
	return Issues.Num() == 0;
}
}

FString FReEchoCsvIssue::ToString() const
{
	return FString::Printf(TEXT("%s:%d:%s: %s"), *File, Line, *Field, *Message);
}

const FReEchoRuntimeSmokeRow* FReEchoCsvDataSnapshot::FindRuntimeSmokeRow(const FName RowId) const
{
	return RuntimeSmokeRows.Find(RowId);
}

FString FReEchoCsvLoadResult::FormatIssues() const
{
	TArray<FString> Lines;
	for (const FReEchoCsvIssue& Issue : Issues)
	{
		Lines.Add(Issue.ToString());
	}
	return FString::Join(Lines, TEXT("\n"));
}

void FReEchoCsvDataRegistry::EnsureDefaultRegistrations()
{
	FScopeLock Lock(&RegistryCriticalSection);
	if (bDefaultRegistrationsReady)
	{
		return;
	}
	RegisteredBehaviorIds.Add(FName(DefaultBehaviorId));
	RegisteredEffectKinds.Add(FName(DefaultEffectKind));
	bDefaultRegistrationsReady = true;
}

void FReEchoCsvDataRegistry::RegisterBehaviorId(const FName BehaviorId)
{
	EnsureDefaultRegistrations();
	if (!BehaviorId.IsNone())
	{
		FScopeLock Lock(&RegistryCriticalSection);
		RegisteredBehaviorIds.Add(BehaviorId);
	}
}

void FReEchoCsvDataRegistry::RegisterEffectKind(const FName EffectKind)
{
	EnsureDefaultRegistrations();
	if (!EffectKind.IsNone())
	{
		FScopeLock Lock(&RegistryCriticalSection);
		RegisteredEffectKinds.Add(EffectKind);
	}
}

bool FReEchoCsvDataRegistry::IsBehaviorIdRegistered(const FName BehaviorId)
{
	EnsureDefaultRegistrations();
	FScopeLock Lock(&RegistryCriticalSection);
	return RegisteredBehaviorIds.Contains(BehaviorId);
}

bool FReEchoCsvDataRegistry::IsEffectKindRegistered(const FName EffectKind)
{
	EnsureDefaultRegistrations();
	FScopeLock Lock(&RegistryCriticalSection);
	return RegisteredEffectKinds.Contains(EffectKind);
}

FString FReEchoCsvDataRegistry::GetDefaultDataDirectory()
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
}

FReEchoCsvLoadResult FReEchoCsvDataRegistry::LoadSnapshotFromDirectory(const FString& DataDirectory)
{
	EnsureDefaultRegistrations();

	FReEchoCsvLoadResult Result;
	TSharedRef<FReEchoCsvDataSnapshot> MutableSnapshot = MakeShared<FReEchoCsvDataSnapshot>();
	MutableSnapshot->SchemaVersion = SupportedSchemaVersion;

	TMap<FString, FReEchoManifestEntry> ManifestEntries;
	ReadManifest(DataDirectory, ManifestEntries, Result.Issues);
	if (Result.Issues.Num() == 0)
	{
		ReadRuntimeSmokeTable(DataDirectory, ManifestEntries[RuntimeSmokeTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0)
	{
		ReadRuntimeSmokeEffectsTable(
		    DataDirectory, ManifestEntries[RuntimeSmokeEffectsTableId], *MutableSnapshot, Result.Issues);
	}
	if (Result.Issues.Num() == 0 && MutableSnapshot->RuntimeSmokeRows.Num() == 0)
	{
		AddIssue(Result.Issues,
		         FPaths::Combine(DataDirectory, ManifestEntries[RuntimeSmokeTableId].FileName),
		         1,
		         TEXT("Id"),
		         TEXT("RuntimeSmoke must contain at least one row"));
	}

	if (Result.Issues.Num() == 0)
	{
		Result.bSuccess = true;
		Result.Snapshot = MutableSnapshot;
	}
	return Result;
}

FReEchoCsvLoadResult FReEchoCsvDataRegistry::LoadAndPublishFromDirectory(const FString& DataDirectory)
{
	FReEchoCsvLoadResult Result = LoadSnapshotFromDirectory(DataDirectory);
	if (Result.bSuccess)
	{
		FScopeLock Lock(&RegistryCriticalSection);
		PublishedSnapshot = Result.Snapshot;
	}
	return Result;
}

FReEchoCsvLoadResult FReEchoCsvDataRegistry::LoadAndPublishDefault()
{
	return LoadAndPublishFromDirectory(GetDefaultDataDirectory());
}

TSharedPtr<const FReEchoCsvDataSnapshot> FReEchoCsvDataRegistry::GetSnapshot()
{
	FScopeLock Lock(&RegistryCriticalSection);
	return PublishedSnapshot;
}

void FReEchoCsvDataRegistry::ClearPublishedSnapshotForTests()
{
	FScopeLock Lock(&RegistryCriticalSection);
	PublishedSnapshot.Reset();
}
