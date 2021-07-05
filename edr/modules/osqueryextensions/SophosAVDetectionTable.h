/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    class SophosAVDetectionTable : public OsquerySDK::TablePluginInterface
    {
    public:
        SophosAVDetectionTable() = default;


        std::string GetName() override;


        std::vector<TableColumn> GetColumns() override;
        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& context) override;

    private:
        std::string getSampleJson() ;

    };
}
