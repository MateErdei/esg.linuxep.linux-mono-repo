/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    class SophosServerTable : public OsquerySDK::TablePluginInterface
    {
    public:
        SophosServerTable() = default;

        std::vector<TableColumn> GetColumns() override;
        std::string GetName() override;
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& request) override;

    };
}
