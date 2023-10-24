// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace livequery
{
    /**
     * Exception raised on the constructor of Response Data if the ColumnData and ColumnHeaders do not hold the
     * validation.
     */
    class InvalidReponseData : std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class ResponseData
    {
        ResponseData();

    public:
        /**
         * The empty case is to be explicit when constructing QueryResponse for error cases for which ColumnHeaders are
         * not available
         * @return
         */
        static ResponseData emptyResponse();

        using ValueType = std::pair<std::string, OsquerySDK::ColumnType>;
        using RowData = std::map<std::string, std::string>;
        using ColumnData = std::vector<RowData>;
        using ColumnHeaders = std::vector<ValueType>;
        enum class MarkDataExceeded
        {
            DataExceedLimit
        };
        /**
         * Constructor will enforce the invariant that each element of the ColumnData has the same RowData keys which
         * are also the same of the ColumnHeaders names
         */
        ResponseData(ColumnHeaders, ColumnData);
        /** Alternative constructor to be used when the columnData exceed the limit imposed of XXX rows **/
        ResponseData(ColumnHeaders, MarkDataExceeded);

        bool hasDataExceededLimit() const;
        bool hasHeaders() const;

        const ColumnData& columnData() const;
        const ColumnHeaders& columnHeaders() const;
        static std::string AcceptedTypesToString(OsquerySDK::ColumnType acceptedType);

    private:
        static bool isValidHeaderAndData(const ColumnHeaders& headers, const ColumnData& data);
        bool m_dataExceedLimit = false;
        ColumnHeaders m_headers;
        ColumnData m_columnData;
    };
} // namespace livequery
