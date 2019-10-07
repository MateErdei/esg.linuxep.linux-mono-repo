/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/PluginApi/ApiException.h>

namespace watchdog
{
    namespace watchdogimpl
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
    } // namespace watchdogimpl
} // namespace watchdog
