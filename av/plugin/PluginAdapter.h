/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "QueueTask.h"
#include "IBaseServiceApi.h"
#include "FileScanner.h"
#include "ScannerSettings.h"
#include "PluginCallback.h"

namespace Example
{
    class PluginAdapter
    {
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        FileScanner m_fileScanner;
        ScannerSettings m_scannerSettings;
    public:
        PluginAdapter(std::shared_ptr<QueueTask> queueTask, std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService, std::shared_ptr<PluginCallback> callback);
        void mainLoop();
    private:
        void runScanner();
        void processPolicy(const std::string & policyXml);
    };
}
