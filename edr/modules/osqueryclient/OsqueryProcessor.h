/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma  once

#include <livequery/IQueryProcessor.h>

namespace osqueryclient{



    class OsqueryProcessor: public livequery::IQueryProcessor {
    public:
        explicit OsqueryProcessor(std::string socketPath);
        livequery::QueryResponse query(const std::string & query) override ;

    private:
        std::string m_socketPath;
    };
}

