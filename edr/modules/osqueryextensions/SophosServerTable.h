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


        std::string GetName() override;
        std::vector<TableColumn> GetColumns() override;
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& context) override;

    };
}
