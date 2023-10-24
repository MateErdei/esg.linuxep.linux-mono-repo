// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once
#include "TablePluginMacros.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

namespace OsquerySDK
{
    class HexToIntTable : public OsquerySDK::TablePluginInterface
    {
    public:
        /*
         * @description
         * Convert a hex string to an integer
         * A hex_string constraint must be provided when querying this table
         *
         * @example
         * SELECT
         *     *
         * FROM
         *     hex_to_int
         * WHERE
         *     hex_string = '0xf'
         */
        TABLE_PLUGIN_NAME(hex_to_int)
        TABLE_PLUGIN_COLUMNS(
            /*
             * @description
             * The hex string to convert
             *
             */
            TABLE_PLUGIN_COLUMN(hex_string, OsquerySDK::TEXT_TYPE, OsquerySDK::ColumnOptions::REQUIRED),
            /*
             * @description
             * The converted integer string
             *
             */
            TABLE_PLUGIN_COLUMN(int, OsquerySDK::TEXT_TYPE, OsquerySDK::ColumnOptions::DEFAULT))

        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& context) override;
    };
}