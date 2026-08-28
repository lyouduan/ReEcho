#pragma once

#include "Data/ReEchoCsvDataRegistry.h"

class UReEchoGameDataCatalog;

namespace ReEchoDataAssetCompiler
{
FReEchoCsvLoadResult Compile(const UReEchoGameDataCatalog& Catalog);
}
