// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

namespace OsquerySDK
{
    class DelayControlledTable : public OsquerySDK::TablePluginInterface
    {
    public:
        DelayControlledTable() = default;

        std::vector<TableColumn>& GetColumns() override;
        std::string& GetName() override;
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& request) override;

    private:
        std::string m_name = { "delaytable" };
        std::vector<OsquerySDK::TableColumn> m_columns = {
            OsquerySDK::TableColumn{"start", BIGINT_TYPE, ColumnOptions::DEFAULT},
            OsquerySDK::TableColumn{"stop", BIGINT_TYPE, ColumnOptions::DEFAULT},
            OsquerySDK::TableColumn{"delay", INTEGER_TYPE, ColumnOptions::REQUIRED}
        };
    };
}
