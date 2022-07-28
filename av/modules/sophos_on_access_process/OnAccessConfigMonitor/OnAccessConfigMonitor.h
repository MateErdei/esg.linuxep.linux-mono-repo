/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"

#include "Common/Threads/AbstractThread.h"

class OnAccessConfigMonitor : public Common::Threads::AbstractThread
{
public:
    explicit OnAccessConfigMonitor(std::string processControllerSocketPath);

    void run() override;
private:
    std::string m_processControllerSocketPath;
    unixsocket::ProcessControllerServerSocket m_processControllerServer;
};
