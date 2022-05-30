/******************************************************************************************************

Copyright 2020-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    class BinaryDataTable : public OsquerySDK::TablePluginInterface
    {
        public:
            BinaryDataTable() = default;

            std::vector<TableColumn>& GetColumns() override;
            std::string& GetName() override;
            OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& request) override;

        private:
            std::string m_name = { "binary_data" };
            std::vector<OsquerySDK::TableColumn> m_columns = {
                OsquerySDK::TableColumn{"data", TEXT_TYPE, ColumnOptions::REQUIRED},
                OsquerySDK::TableColumn{"size", INTEGER_TYPE, ColumnOptions::REQUIRED}
            };
    };
}