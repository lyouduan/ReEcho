#pragma once

#include "Data/ReEchoCsvDataRegistry.h"

namespace ReEchoCsv
{
struct FRow
{
	int32 Line = 0;
	TMap<FString, FString> Cells;
};

struct FTable
{
	FString File;
	TArray<FString> Headers;
	TArray<FRow> Rows;
};

struct FManifestEntry
{
	FString TableId;
	FString FileName;
	FString PrimaryKey;
	int32 SchemaVersion = 0;
	int32 Line = 0;
};

void AddIssue(
    TArray<FReEchoCsvIssue>& Issues, const FString& File, int32 Line, const FString& Field, const FString& Message);

bool ParseCsvFile(const FString& FilePath, FTable& OutTable, TArray<FReEchoCsvIssue>& Issues);
bool HasExactColumns(const FTable& Table, const TArray<FString>& ExpectedColumns, TArray<FReEchoCsvIssue>& Issues);

bool RequireCell(
    const FTable& Table, const FRow& Row, const FString& Field, FString& OutValue, TArray<FReEchoCsvIssue>& Issues);
bool ReadOptionalCell(const FRow& Row, const FString& Field, FString& OutValue);
bool RequireStableId(
    const FTable& Table, const FRow& Row, const FString& Field, FName& OutValue, TArray<FReEchoCsvIssue>& Issues);
bool RequireBool(
    const FTable& Table, const FRow& Row, const FString& Field, bool& OutValue, TArray<FReEchoCsvIssue>& Issues);
bool RequireFloat(const FTable& Table,
                  const FRow& Row,
                  const FString& Field,
                  float MinValue,
                  float MaxValue,
                  float& OutValue,
                  TArray<FReEchoCsvIssue>& Issues);
bool RequireInt(
    const FTable& Table, const FRow& Row, const FString& Field, int32& OutValue, TArray<FReEchoCsvIssue>& Issues);
bool RequireValueOp(const FTable& Table,
                    const FRow& Row,
                    const FString& Field,
                    EReEchoCsvValueOp& OutValue,
                    TArray<FReEchoCsvIssue>& Issues);

bool ReadManifest(const FString& DataDirectory,
                  const TArray<FString>& RequiredTables,
                  TMap<FString, FManifestEntry>& OutEntries,
                  TArray<FReEchoCsvIssue>& Issues);
}
