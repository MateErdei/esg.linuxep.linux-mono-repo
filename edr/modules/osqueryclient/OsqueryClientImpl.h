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
        osquery::Status query(const std::string& sql, osquery::QueryData& qd) override;
        osquery::Status getQueryColumns(const std::string& sql, osquery::QueryData& qd) override;

    private:
        std::unique_ptr<osquery::ExtensionManagerAPI> m_client;
    };
} // namespace osqueryclient
