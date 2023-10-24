// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "TablePluginMacros.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

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
            TABLE_PLUGIN_COLUMN(customer_id, TEXT_TYPE, ColumnOptions::DEFAULT),

            /*
             * @description
             * Json array containing the installed Sophos component versions
             * [
             *     {"Name": "LiveQuery64", "installed_version": "1.2.3.4"},
             *     {"Name": "Component2", "installed_version": "5.6.7.8"}
             * ]
             */
            TABLE_PLUGIN_COLUMN(installed_versions, TEXT_TYPE, ColumnOptions::DEFAULT))

        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& context) override;
    };
} // namespace OsquerySDK
