/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "StopAction.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>

using namespace wdctl::wdctlactions;

StopAction::StopAction(const wdctl::wdctlarguments::Arguments& args)
        : ZMQAction(args)
{
}

int StopAction::run()
{
    LOGINFO("Attempting to stop "<<m_args.m_argument);

    auto response = doOperationToWatchdog({"STOP",m_args.m_argument});

    if (isSuccessful(response))
    {
        return 0;
    }

    LOGERROR("Failed to stop "<< m_args.m_argument<<": "<<response.at(0));
    return 1;
}
