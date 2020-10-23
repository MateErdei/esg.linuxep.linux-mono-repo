/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ResponseData.h"
#include "Logger.h"
#include <unordered_set>
namespace livequery
{
    ResponseData::ResponseData() = default;

    ResponseData ResponseData::emptyResponse()
    {
        return ResponseData {};
    }

    ResponseData::ResponseData(ResponseData::ColumnHeaders headers, ResponseData::ColumnData data) :
        m_headers(std::move(headers)),
        m_columnData(std::move(data))
    {
        if (!isValidHeaderAndData(m_headers, m_columnData))
        {
            throw InvalidReponseData { "Headers and data provided are not coherent." };
        }
    }

    ResponseData::ResponseData(ResponseData::ColumnHeaders headers, ResponseData::MarkDataExceeded) :
        m_dataExceedLimit { true },
        m_headers { std::move(headers) },
        m_columnData {}
    {
    }

    bool ResponseData::hasDataExceededLimit() const
    {
        return m_dataExceedLimit;
    }

    bool ResponseData::hasHeaders() const
    {
        return !m_headers.empty();
    }

    const ResponseData::ColumnData& ResponseData::columnData() const
    {
        return m_columnData;
    }

    const ResponseData::ColumnHeaders& ResponseData::columnHeaders() const
    {
        return m_headers;
    }

    /**
     * A valid responsedata requires that no entry in the column data is not defined in the headers.
     * @return
     */
    bool ResponseData::isValidHeaderAndData(
        const ResponseData::ColumnHeaders& headers,
        const ResponseData::ColumnData& data)
    {
        std::unordered_set<std::string> columnsReported;
        for (const auto& row : headers)
        {
            const std::string & columnName = row.first;
            columnsReported.insert(columnName);
        }
        int row=0;
        for (auto& columnsInRow : data)
        {
            for( auto & cellInRow : columnsInRow)
            {
                if (columnsReported.find(cellInRow.first) == columnsReported.end())
                {
                    LOGWARN("Error in row: " << row << ". Unexpected value named (" << cellInRow.first << ")");
                    return false;
                }
            }
            row++;
        }
        return true;
    }

    std::string ResponseData::AcceptedTypesToString(OsquerySDK::ColumnType acceptedType)
    {
        switch (acceptedType)
        {
            case OsquerySDK::ColumnType::BIGINT_TYPE:
                return "BIGINT";
            case OsquerySDK::ColumnType::INTEGER_TYPE:
                return "INTEGER";
            case OsquerySDK::ColumnType::UNSIGNED_BIGINT_TYPE:
                return "UNSIGNED BIGINT";
            case OsquerySDK::ColumnType::BLOB_TYPE:
                return "BLOB";
            case OsquerySDK::ColumnType::DOUBLE_TYPE:
                return "DOUBLE";
            default:
            case OsquerySDK::ColumnType::TEXT_TYPE:
                return "TEXT";
        }
    }

} // namespace livequery