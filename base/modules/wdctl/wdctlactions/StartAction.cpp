/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "StartAction.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>

using namespace wdctl::wdctlactions;

StartAction::StartAction(const wdctl::wdctlarguments::Arguments& args)
        : ZMQAction(args)
{
}

int StartAction::run()
{
    LOGINFO("Attempting to start "<<m_args.m_argument);

    Common::ZeroMQWrapper::ISocketRequesterPtr socket = m_context->getRequester();
    socket->connect(Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress());

    socket->write({"START",m_args.m_argument});

    auto response = socket->read();

    if (response.size() == 1 && response.at(0) == "OK")
    {
        return 0;
    }

    LOGERROR("Failed to start "<< m_args.m_argument<<": "<<response.at(0));
    return 1;
}
