/******************************************************************************************************

Copyright 2020-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    class MemoryCrashTable : public OsquerySDK::TablePluginInterface
    {
    public:
        MemoryCrashTable() = default;

        std::vector<TableColumn>& GetColumns() override;
        std::string& GetName() override;
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& request) override;
    private:
        std::string m_name = { "memorycrashtable" };
        std::vector<OsquerySDK::TableColumn> m_columns = {
            OsquerySDK::TableColumn{"string", INTEGER_TYPE, ColumnOptions::DEFAULT}
        };
    };
}
