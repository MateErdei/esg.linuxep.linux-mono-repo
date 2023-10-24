// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "OsqueryClientImpl.h"

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

#include <thrift/transport/TTransportException.h>

#include<thread>

namespace osquery
{
    std::unique_ptr<OsquerySDK::OsqueryClientInterface> makeClient(const std::string& socket)
    {
        for (int i = 0; i < 15; i++)
        {
            try
            {
                // set a maximum time that a query can be take. Set to 90 minutes.
                constexpr int Timeout=90*60;
                std::chrono::seconds timeout = std::chrono::seconds(Timeout);
                return OsquerySDK::CreateOsqueryClient(socket, timeout);
            }
            catch (apache::thrift::transport::TTransportException& ex)
            {
                LOGINFO("Connection to osquery failed: " << ex.what());
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        throw osqueryclient::FailedToStablishConnectionException("Could not connect to osquery after 3 seconds");
    }
} // namespace osquery

namespace osqueryclient
{
    void OsqueryClientImpl::connect(const std::string& socketPath)
    {
        m_client = osquery::makeClient(socketPath);
    }

    OsquerySDK::Status OsqueryClientImpl::query(const std::string& sql, OsquerySDK::QueryData& qd)
    {
        assert(m_client);
        return m_client->Query(sql, qd);
    }

    OsquerySDK::Status OsqueryClientImpl::getQueryColumns(const std::string& sql, OsquerySDK::QueryColumns& qc)
    {
        assert(m_client);
        return m_client->GetQueryColumns(sql, qc);
    }
} // namespace osqueryclient

Common::UtilityImpl::Factory<osqueryclient::IOsqueryClient>& osqueryclient::factory()
{
    static Common::UtilityImpl::Factory<IOsqueryClient> theFactory { []() {
        return std::unique_ptr<IOsqueryClient>(new osqueryclient::OsqueryClientImpl());
    } };
    return theFactory;
}
