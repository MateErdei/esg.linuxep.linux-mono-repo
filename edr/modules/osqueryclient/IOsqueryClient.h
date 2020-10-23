/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <OsquerySDK/OsquerySDK.h>
#include <Common/UtilityImpl/Factory.h>
namespace osqueryclient
{
    class FailedToStablishConnectionException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class IOsqueryClient
    {
    public:
        virtual ~IOsqueryClient() = default;

        virtual void connect(const std::string& socketPath) = 0;
        virtual OsquerySDK::Status query(const std::string& sql, OsquerySDK::QueryData& qd) = 0;
        virtual OsquerySDK::Status getQueryColumns(const std::string& sql, OsquerySDK::QueryColumns & qc) = 0;
    };

    Common::UtilityImpl::Factory<IOsqueryClient>& factory();
} //namespace osqueryclient
