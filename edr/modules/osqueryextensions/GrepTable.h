// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "TablePluginMacros.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif


namespace
{
    const unsigned int MAX_LINE_LENGTH = 1000;
} // namespace
namespace OsquerySDK
{
    class GrepTable : public OsquerySDK::TablePluginInterface
    {
    public:
        GrepTable() = default;
        /*
     * @description
     * Search a given path for a given pattern
     *
     * @example
     * SELECT
     *     *
     * FROM
     *     grep
     * WHERE
     *     path = 'C:\test\file.txt'
     * AND
     *     pattern = 'TestPattern'
         */
        TABLE_PLUGIN_NAME(grep)
        TABLE_PLUGIN_COLUMNS(
            /*
         * @description
         * The path to a directory or file to search
         *
             */
            TABLE_PLUGIN_COLUMN(path, OsquerySDK::TEXT_TYPE, OsquerySDK::ColumnOptions::REQUIRED),
            /*
         * @description
         * The path to a file that contains the search string
         *
             */
            TABLE_PLUGIN_COLUMN(filepath, OsquerySDK::TEXT_TYPE, OsquerySDK::ColumnOptions::DEFAULT),
            /*
         * @description
         * The search string
         *
             */
            TABLE_PLUGIN_COLUMN(pattern, OsquerySDK::TEXT_TYPE, OsquerySDK::ColumnOptions::DEFAULT),
            /*
         * @description
         * The full line in the file that contains the 'pattern'
         *
             */
            TABLE_PLUGIN_COLUMN(line, OsquerySDK::TEXT_TYPE, OsquerySDK::ColumnOptions::DEFAULT))


        OsquerySDK::TableRows Generate(OsquerySDK::QueryContextInterface& context) override;

    private:
        void GrepFolder(
            const std::string& path,
            const std::string& grepPath,
            const std::string& pattern,
            OsquerySDK::TableRows& results,
            OsquerySDK::QueryContextInterface& context);

        void GrepFile(
            const std::string& path,
            const std::string& filePath,
            const std::string& pattern,
            OsquerySDK::TableRows& results,
            OsquerySDK::QueryContextInterface& context);


    };
}