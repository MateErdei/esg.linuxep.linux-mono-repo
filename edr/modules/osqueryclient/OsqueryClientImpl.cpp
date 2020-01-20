/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryClientImpl.h"
#include <osquery/flags.h>
#include <thrift/transport/TTransportException.h>
#include "Logger.h"

namespace osquery
{

    std::unique_ptr<osquery::ExtensionManagerAPI> makeClient(const std::string& socket)
    {
        for( int i = 0; i<15; i++)
        {
            try{
                return std::make_unique<osquery::ExtensionManagerClient>(socket, 10000);
            }catch(apache::thrift::transport::TTransportException & ex)
            {
                LOGINFO("Connection to osquery failed: " << ex.what());
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        throw  osqueryclient::FailedToStablishConnectionException("Could not connect to osquery after 3 seconds");
    }
}


namespace osqueryclient
{

    void OsqueryClientImpl::connect(const std::string& socketPath)
    {
        m_client = osquery::makeClient(socketPath);
    }

    osquery::Status OsqueryClientImpl::query(const std::string& sql, osquery::QueryData& qd)
    {
        assert(m_client);
        return m_client->query(sql, qd);
    }

    osquery::Status OsqueryClientImpl::getQueryColumns(const std::string& sql, osquery::QueryData& qd)
    {
        assert(m_client);
        return m_client->getQueryColumns(sql, qd);
    }
}

Common::UtilityImpl::Factory<osqueryclient::IOsqueryClient>& osqueryclient::factory()
{
    static Common::UtilityImpl::Factory<IOsqueryClient> theFactory{ [](){
        return std::unique_ptr<IOsqueryClient>(new osqueryclient::OsqueryClientImpl());
    } };
    return theFactory;
}
