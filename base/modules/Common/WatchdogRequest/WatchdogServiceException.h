// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/PluginApi/ApiException.h"

namespace Common::WatchdogRequest
{
    class WatchdogServiceException : public Common::PluginApi::ApiException
    {
    public:
        using ApiException::ApiException;
    };

    class UpdateServiceReportError : public WatchdogServiceException
    {
    public:
        static std::string ErrorReported() { return "Update service reported error"; }

        UpdateServiceReportError() : WatchdogServiceException(ErrorReported()) {}
    };
} // namespace Common::WatchdogRequest
