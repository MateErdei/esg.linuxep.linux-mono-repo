/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ResponseData.h"
#include <unordered_set>
#include "Logger.h"
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

    std::string ResponseData::AcceptedTypesToString(livequery::ResponseData::AcceptedTypes acceptedType)
    {
        switch (acceptedType)
        {
            case livequery::ResponseData::AcceptedTypes::BIGINT:
                return "BIGINT";
            case livequery::ResponseData::AcceptedTypes::DATE:
                return "DATE";
            case livequery::ResponseData::AcceptedTypes::DATETIME:
                return "DATETIME";
            case livequery::ResponseData::AcceptedTypes::INTEGER:
                return "INTEGER";
            case livequery::ResponseData::AcceptedTypes::UNSIGNED_BIGINT:
                return "UNSIGNED BIGINT";
            default:
            case livequery::ResponseData::AcceptedTypes::STRING:
                return "TEXT";
        }
    }

    ResponseData::AcceptedTypes ResponseData::AcceptedTypesFromString(const std::string& acceptedTypeStr)
    {
        static std::unordered_map<std::string, ResponseData::AcceptedTypes > mapString2Type{
                {"TEXT", ResponseData::AcceptedTypes::STRING},
                { "BIGINT", ResponseData::AcceptedTypes::BIGINT},
                {"INTEGER", ResponseData::AcceptedTypes::INTEGER},
                {"UNSIGNED LONG", ResponseData::AcceptedTypes::UNSIGNED_BIGINT},
                {"DATE", ResponseData::AcceptedTypes::DATE},
                {"DATETIME", ResponseData::AcceptedTypes::DATETIME}
        };

        auto found = mapString2Type.find(acceptedTypeStr);
        if (found == mapString2Type.end())
        {
            return ResponseData::AcceptedTypes::STRING;
        }
        return found->second;
    }
} // namespace livequery