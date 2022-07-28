/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"

#include "Common/Threads/AbstractThread.h"

namespace sophos_on_access_process::ConfigMonitorThread
{
    class OnAccessConfigMonitor : public Common::Threads::AbstractThread
    {
    public:
        explicit OnAccessConfigMonitor(std::string processControllerSocketPath);

        void run() override;

        static std::string readConfigFile();
        static bool parseOnAccessSettingsFromJson(const std::string& jsonString);

    private:
        std::string m_processControllerSocketPath;
        unixsocket::ProcessControllerServerSocket m_processControllerServer;
        static bool isSettingTrue(const std::string& settingString);
    };
}