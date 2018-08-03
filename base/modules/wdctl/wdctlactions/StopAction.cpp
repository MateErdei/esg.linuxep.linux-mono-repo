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

StopAction::StopAction(const wdctl::wdctlimpl::Arguments& args)
        : ZMQAction(args)
{
}

int StopAction::run()
{
    LOGINFO("Attempting to stop "<<m_args.m_argument);

    Common::ZeroMQWrapper::ISocketRequesterPtr socket = m_context->getRequester();
    socket->connect(Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress());

    socket->write({"STOP",m_args.m_argument});

    auto response = socket->read();

    if (response.size() == 1 && response.at(0) == "OK")
    {
        return 0;
    }

    LOGERROR("Failed to stop "<< m_args.m_argument);
    return 1;
}
