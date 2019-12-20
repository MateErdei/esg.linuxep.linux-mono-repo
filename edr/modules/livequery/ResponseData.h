/******************************************************************************************************

Copyright Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma  once

#include <string>
#include <map>
#include <vector>

namespace livequery{
    class ResponseData {
    public:
        using ValueType = std::pair<std::string, std::string>;
        using RowData = std::map<std::string, std::string>;
        using ColumnData = std::vector<RowData>;
        using ColumnHeaders = std::vector<ValueType>;
        /**
         * Constructor will enforce the invariant that each element of the ColumnData has the same RowData keys which are also the same of the ColumnHeaders names
         */
        ResponseData( ColumnHeaders, ColumnData);
        const ColumnData & columnData() const;
        const ColumnHeaders & columnHeaders() const;
    };
}
