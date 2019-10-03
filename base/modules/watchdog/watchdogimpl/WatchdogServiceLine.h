/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <Common/PluginApiImpl/PluginCallBackHandler.h>
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
            static std::string ErrorReported;
            UpdateServiceReportError():WatchdogServiceException(ErrorReported)
            {

            }

        };

        class WatchdogServiceLine
        {
        public:
            static std::string WatchdogServiceLineName;
            static void requestUpdateService(Common::ZMQWrapperApi::IContext & );
            static void requestUpdateService( );
            WatchdogServiceLine( Common::ZMQWrapperApi::IContextSharedPtr );
            ~WatchdogServiceLine();

        private:
            Common::ZMQWrapperApi::IContextSharedPtr m_context;
            std::unique_ptr<Common::PluginApiImpl::PluginCallBackHandler> m_pluginHandler;
        };
    }
}

