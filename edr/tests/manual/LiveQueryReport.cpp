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
#include <livequery/IQueryProcessor.h>
#include <osqueryclient/OsqueryProcessor.h>
#include <thirdparty/nlohmann-json/json.hpp>
#include <Common/Logging/ConsoleLoggingSetup.h>

namespace osquery{

    FLAG(bool, decorations_top_level, false, "test");
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
namespace livequery
{
    class ShowInConsoleDispatcher : public livequery::ResponseDispatcher
    {
    public:
        void sendResponse(const std::string& /*correlationId*/, const QueryResponse& response) override
        {
            auto content = serializeToJson(response);
            nlohmann::json  jsoncontent = nlohmann::json::parse(content);
            std::cout << jsoncontent.dump(2) << std::endl;
        }
    };

}

int main(int argc, char * argv[])
{
    assert(argc==3);
    if(argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " [socketpath] [query]" << std::endl;
        return 2;
    }
    Common::Logging::ConsoleLoggingSetup loggingSetup;
    std::string m_socketPath{argv[1]};
    std::string query{argv[2]};
    livequery::ShowInConsoleDispatcher showInConsoleDispatcher;
    osqueryclient::OsqueryProcessor osqueryProcessor{m_socketPath};

    nlohmann::json queryStructure;
    queryStructure["type"] = "sophos.mgt.action.RunLiveQuery";
    queryStructure["name"] = "";
    queryStructure["query"] = query;

    livequery::processQuery(osqueryProcessor, showInConsoleDispatcher,
            queryStructure.dump(), "anycorrelation" );
    return 0;
}
