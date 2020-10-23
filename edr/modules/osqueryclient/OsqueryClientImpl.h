/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IOsqueryClient.h"

namespace osqueryclient
{
    class OsqueryClientImpl : public virtual IOsqueryClient
    {
    public:
        ~OsqueryClientImpl() = default;

        void connect(const std::string& socketPath) override;
        OsquerySDK::Status query(const std::string& sql, OsquerySDK::QueryData& qd) override;
        OsquerySDK::Status getQueryColumns(const std::string& sql, OsquerySDK::QueryColumns& qc) override;

    private:
        std::unique_ptr<OsquerySDK::OsqueryClientInterface>  m_client;
    };
} // namespace osqueryclient
