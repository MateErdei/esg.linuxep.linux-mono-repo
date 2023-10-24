// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

#include "Common/UtilityImpl/Factory.h"

#include <stdexcept>

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
        virtual OsquerySDK::Status getQueryColumns(const std::string& sql, OsquerySDK::QueryColumns& qc) = 0;
    };

    Common::UtilityImpl::Factory<IOsqueryClient>& factory();
} //namespace osqueryclient
