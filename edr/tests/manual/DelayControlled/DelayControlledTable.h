/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    class DelayControlledTable : public OsquerySDK::TablePluginInterface
    {
    public:
        DelayControlledTable() = default;

        std::vector<TableColumn> GetColumns() override;
        std::string GetName() override;
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& request) override;

    };
}
