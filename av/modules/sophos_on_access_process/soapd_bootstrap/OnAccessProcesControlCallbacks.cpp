// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessProcesControlCallbacks.h"

#include "Logger.h"

namespace sophos_on_access_process::OnAccessConfig
{
    void OnAccessProcessControlCallback::processControlMessage(const scan_messages::E_COMMAND_TYPE& command)
    {
        switch (command)
        {
            case scan_messages::E_SHUTDOWN:
                LOGINFO("Dismissing request to shutdown on-access");
                break;
            case scan_messages::E_RELOAD:
                LOGINFO("Sophos On Access Process received configuration reload request");
                m_soapd.ProcessPolicy();
                break;
            default:
                LOGWARN("Sophos On Access Process received unknown process control message");
        }
    }
}


