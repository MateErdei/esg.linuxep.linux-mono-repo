/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "PluginCallback.h"
#include "ITaskQueue.h"
#include "IAsyncDiagnoseRunner.h"

#include <Common/DirectoryWatcher/IiNotifyWrapper.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/Threads/AbstractThread.h>

#include <functional>
#include <future>
#include <optional>
#include <map>
#include <memory>

namespace RemoteDiagnoseImpl
{
    class PluginUtils
    {

    public:
        struct UrlData
        {
            std::string domain;
            std::string resourcePath;
            std::string filename;
            int port = 0;
        };
        PluginUtils();

        ~PluginUtils();


        static std::string processAction(const std::string& actionXml);
        static std::string getFinishedStatus();
        static void processZip(const std::string& url);
        static UrlData processUrl(const std::string& url);
    };


} // namespace RemoteDiagnoseImpl