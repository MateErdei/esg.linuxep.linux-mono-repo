/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "TablePluginMacros.h"

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    class SophosServerTable : public OsquerySDK::TablePluginInterface
    {
    public:
        SophosServerTable() = default;

        /*
         * @description
         * Gets Sophos endpoint information. (For internal use only)
         *
         * @example
         * SELECT
         *     endpoint_id
         * FROM
         *     sophos_endpoint_info
         */
        TABLE_PLUGIN_NAME(sophos_endpoint_info)
        TABLE_PLUGIN_COLUMNS(
            /*
             * @description
             * The endpoint id of the machine
             */
            TABLE_PLUGIN_COLUMN(endpoint_id, TEXT_TYPE, ColumnOptions::DEFAULT),
            /*
             * @description
             * The customer id stored on the machine
             */
            TABLE_PLUGIN_COLUMN(customer_id, TEXT_TYPE, ColumnOptions::DEFAULT))

        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& context) override;

    };
}
