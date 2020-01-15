/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <string>
#include <livequery/ResponseData.h>
#include <livequery/ResponseStatus.h>
#include <extensions/interface.h>
#include <osquery/flags.h>
#include <osquery/flagalias.h>
#include <iostream>
namespace osquery{

    FLAG(bool, decorations_top_level, false, "test");
    std::unique_ptr<osquery::ExtensionManagerAPI> makeClient(std::string socket)
    {
        return std::make_unique<osquery::ExtensionManagerClient>(socket, 10000);
    }
}

/**
 * Usage:
 * LiveQueryReport  'sockpath'  'query'
 * Example:
 * LiveQueryReport /tmp/osquery.sock 'select * from osquery_flags'
 * @param argc
 * @param argv
 * @return
 */

int main(int argc, char * argv[])
{
    assert(argc==3);
    if(argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " [socketpath] [query]" << std::endl;
        return 2;
    }
    std::string m_socketPath{argv[1]};
    std::string query{argv[2]};

    osquery::QueryData qd;
    auto client = osquery::makeClient(m_socketPath);
    auto osqueryStatus = client->query(query, qd);
    if ( !osqueryStatus.ok())
    {
        std::cerr << "Query failed: " << osqueryStatus.getMessage() << std::endl;
        return 1;
    }
    osquery::QueryData qcolumn;
    osqueryStatus = client->getQueryColumns(query, qcolumn);
    if ( !osqueryStatus.ok())
    {
        std::cerr << "Query column failed: " << osqueryStatus.getMessage() << std::endl;
        return 1;
    }

    livequery::ResponseData::ColumnData  columnData = qd;
    livequery::ResponseData::ColumnHeaders headers;

    std::cout << "Columns: \n";
    for (const auto& row : qcolumn)
    {
        const std::string & columnName = row.begin()->first;
        const std::string & columnType = row.begin()->second;
        std::cout <<  "Name: " << columnName << "\t Type: " << columnType << '\n';
    }
    std::cout << std::endl;
    std::cout << "Data: \n";
    for(auto & row: columnData)
    {
        for( auto & columnEntry: row)
        {
            std::cout << columnEntry.first << ":" << columnEntry.second << '\t';
        }
        std::cout << std::endl;
    }
    return 0;
}
