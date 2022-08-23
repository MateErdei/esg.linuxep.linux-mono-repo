/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"

#include "common/AbstractThreadPluginInterface.h"

namespace sophos_on_access_process::OnAccessConfig
{
    class OnAccessConfigMonitor : public common::AbstractThreadPluginInterface
    {
    public:
        explicit OnAccessConfigMonitor(std::string processControllerSocketPath,
                                       Common::Threads::NotifyPipe& pipe);

        void run() override;

    private:
        std::string m_processControllerSocketPath;
        unixsocket::ProcessControllerServerSocket m_processControllerServer;
        Common::Threads::NotifyPipe& m_configChangedPipe;
    };
}