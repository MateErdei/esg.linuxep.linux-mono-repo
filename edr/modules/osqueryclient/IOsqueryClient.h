/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <extensions/interface.h>
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
        virtual osquery::Status query(const std::string& sql, osquery::QueryData& qd) = 0;
        virtual osquery::Status getQueryColumns(const std::string& sql, osquery::QueryData& qd) = 0;
    };

    Common::UtilityImpl::Factory<IOsqueryClient>& factory();
} //namespace osqueryclient