/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    class MemoryCrashTable : public OsquerySDK::TablePluginInterface
    {
    public:
        MemoryCrashTable() = default;

        std::vector<TableColumn> GetColumns() override;
        std::string GetName() override;
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& request) override;

    };
}