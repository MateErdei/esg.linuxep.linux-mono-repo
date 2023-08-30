// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginCallback.h"

#include "Common/PluginApi/IBaseServiceApi.h"
#include "taskQueue/ITaskQueue.h"

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>

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
        /*
         * @brief  Retrieves the status of the SDU, updating the is_running field based on the parameter.
         * @param  isRunning - 1 (diagnose is running), 0 (diagnose is not running)
         * @return  string - XML status message
         */
        static std::string getStatus(int isRunning);
        static void processZip(const std::string& url);
        static UrlData processUrl(const std::string& url);
    };

} // namespace RemoteDiagnoseImpl