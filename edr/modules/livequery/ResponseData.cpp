/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ResponseData.h"
#include "Logger.h"
namespace livequery
{

    ResponseData::ResponseData()
    {

    }

    ResponseData ResponseData::emptyResponse()
    {
        return ResponseData{};
    }

    ResponseData::ResponseData(ResponseData::ColumnHeaders headers, ResponseData::ColumnData data):
            m_headers(std::move(headers)), m_columnData(std::move(data))
    {
        if ( !isValidHeaderAndData(m_headers, m_columnData))
        {
            throw InvalidReponseData{"Headers and data provided are not coherent."};
        }
    }

    ResponseData::ResponseData(ResponseData::ColumnHeaders headers, ResponseData::MarkDataExceeded):
            m_dataExceedLimit{true}, m_headers{std::move(headers)}, m_columnData{}
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

    const ResponseData::ColumnData &ResponseData::columnData() const
    {
        return m_columnData;
    }

    const ResponseData::ColumnHeaders &ResponseData::columnHeaders() const
    {
        return m_headers;
    }

    bool ResponseData::isValidHeaderAndData(const ResponseData::ColumnHeaders & headers,
            const ResponseData::ColumnData & data)
    {
        int row = 0;
        for( auto & columnsInRow:  data)
        {
            if ( columnsInRow.size() != headers.size())
            {
                LOGWARN( "Error in row: " << row << ". Number of entries (" << columnsInRow.size() <<
                ") is different from the expected (" << headers.size() << ")");
                return false;
            }
            for( auto & headerEntry: headers)
            {
                if( columnsInRow.count(headerEntry.first) != 1)
                {
                    LOGWARN( "Error in row: " << row << ". Missing at least element (" << headerEntry.first << ")");

                    return false;
                }
            }
            row++;
        }
        return true;
    }
}