#pragma once

#include "CoreMinimal.h"

enum class EReEchoCsvValueOp : uint8
{
	Add,
	Multiply,
	Override
};

struct REECHO_API FReEchoCsvIssue
{
	FString File;
	int32 Line = 0;
	FString Field;
	FString Message;

	FString ToString() const;
};

struct REECHO_API FReEchoRuntimeSmokeEffectRow
{
	FName Id;
	FName RuntimeRowId;
	FName ParamName;
	EReEchoCsvValueOp ValueOp = EReEchoCsvValueOp::Add;
	float Value = 0.0f;
};

struct REECHO_API FReEchoRuntimeSmokeRow
{
	FName Id;
	FString DisplayNameKey;
	bool bEnabled = false;
	float TestScalar = 0.0f;
	float TestPercent = 0.0f;
	float DistanceCm = 0.0f;
	float DurationSeconds = 0.0f;
	FName BehaviorId;
	FName EffectKind;
	EReEchoCsvValueOp ModifierOp = EReEchoCsvValueOp::Override;
	TArray<FReEchoRuntimeSmokeEffectRow> Effects;
};

struct REECHO_API FReEchoCsvDataSnapshot
{
	int32 SchemaVersion = 0;
	TMap<FName, FReEchoRuntimeSmokeRow> RuntimeSmokeRows;

	const FReEchoRuntimeSmokeRow* FindRuntimeSmokeRow(FName RowId) const;
};

struct REECHO_API FReEchoCsvLoadResult
{
	bool bSuccess = false;
	TSharedPtr<const FReEchoCsvDataSnapshot> Snapshot;
	TArray<FReEchoCsvIssue> Issues;

	FString FormatIssues() const;
};

class REECHO_API FReEchoCsvDataRegistry
{
public:
	static constexpr int32 SupportedSchemaVersion = 1;

	static void RegisterBehaviorId(FName BehaviorId);
	static void RegisterEffectKind(FName EffectKind);
	static bool IsBehaviorIdRegistered(FName BehaviorId);
	static bool IsEffectKindRegistered(FName EffectKind);

	static FString GetDefaultDataDirectory();
	static FReEchoCsvLoadResult LoadSnapshotFromDirectory(const FString& DataDirectory);
	static FReEchoCsvLoadResult LoadAndPublishFromDirectory(const FString& DataDirectory);
	static FReEchoCsvLoadResult LoadAndPublishDefault();
	static TSharedPtr<const FReEchoCsvDataSnapshot> GetSnapshot();
	static void ClearPublishedSnapshotForTests();

private:
	static void EnsureDefaultRegistrations();
};
