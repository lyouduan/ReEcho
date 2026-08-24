#include "ReEchoCsvDataReader.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ReEchoCsv
{
namespace
{
constexpr const TCHAR* ManifestFileName = TEXT("reecho_data_manifest.csv");

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

bool ParseCsvFile(const FString& FilePath, FTable& OutTable, TArray<FReEchoCsvIssue>& Issues)
{
	const int32 OriginalIssueCount = Issues.Num();
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

		FRow Row;
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
	return Issues.Num() == OriginalIssueCount;
}

bool HasExactColumns(const FTable& Table, const TArray<FString>& ExpectedColumns, TArray<FReEchoCsvIssue>& Issues)
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

bool RequireCell(
    const FTable& Table, const FRow& Row, const FString& Field, FString& OutValue, TArray<FReEchoCsvIssue>& Issues)
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

bool ReadOptionalCell(const FRow& Row, const FString& Field, FString& OutValue)
{
	const FString* Found = Row.Cells.Find(Field);
	OutValue = Found ? *Found : FString();
	return Found && !OutValue.IsEmpty();
}

bool RequireStableId(
    const FTable& Table, const FRow& Row, const FString& Field, FName& OutValue, TArray<FReEchoCsvIssue>& Issues)
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

bool RequireBool(
    const FTable& Table, const FRow& Row, const FString& Field, bool& OutValue, TArray<FReEchoCsvIssue>& Issues)
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

bool RequireFloat(const FTable& Table,
                  const FRow& Row,
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

bool RequireInt(
    const FTable& Table, const FRow& Row, const FString& Field, int32& OutValue, TArray<FReEchoCsvIssue>& Issues)
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

bool RequireValueOp(const FTable& Table,
                    const FRow& Row,
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
                  const TArray<FString>& RequiredTables,
                  TMap<FString, FManifestEntry>& OutEntries,
                  TArray<FReEchoCsvIssue>& Issues)
{
	FTable Manifest;
	const FString ManifestPath = FPaths::Combine(DataDirectory, ManifestFileName);
	if (!ParseCsvFile(ManifestPath, Manifest, Issues))
	{
		return false;
	}

	HasExactColumns(Manifest, {TEXT("SchemaVersion"), TEXT("TableId"), TEXT("FileName"), TEXT("PrimaryKey")}, Issues);
	TSet<FString> SeenTables;
	for (const FRow& Row : Manifest.Rows)
	{
		FManifestEntry Entry;
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
		// The manifest is shared by module-owned data domains. This gameplay
		// registry only owns RequiredTables; external tables (for example the
		// ReEchoAudio catalog keyed by EventId) validate their primary key in
		// their own module/tooling and must not make gameplay startup fatal.
		FString ExpectedPrimaryKey = TEXT("Id");
		if (Entry.TableId == TEXT("shop_price_ranges"))
		{
			ExpectedPrimaryKey = TEXT("PriceCategory");
		}
		else if (Entry.TableId == TEXT("shop_drop_levels") || Entry.TableId == TEXT("EnemyShardDrops"))
		{
			ExpectedPrimaryKey = TEXT("EncounterIndex");
		}
		if (RequiredTables.Contains(Entry.TableId) && Entry.PrimaryKey != ExpectedPrimaryKey)
		{
			AddIssue(Issues,
			         Manifest.File,
			         Row.Line,
			         TEXT("PrimaryKey"),
			         FString::Printf(TEXT("PrimaryKey must be %s"), *ExpectedPrimaryKey));
		}
		if (Entry.FileName.Contains(TEXT("/")) || Entry.FileName.Contains(TEXT("\\")))
		{
			AddIssue(Issues,
			         Manifest.File,
			         Row.Line,
			         TEXT("FileName"),
			         TEXT("FileName must stay inside its data directory"));
		}
		SeenTables.Add(Entry.TableId);
		OutEntries.Add(Entry.TableId, Entry);
	}

	for (const FString& RequiredTable : RequiredTables)
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
}
