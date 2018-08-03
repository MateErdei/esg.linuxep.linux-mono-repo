/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "StopAction.h"
#include "Logger.h"

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
    Common::ZeroMQWrapper::ISocketRequesterPtr socket = m_context->getRequester();

    LOGINFO("Attempting to stop "<<m_args.m_argument);

    socket->write({"STOP",m_args.m_argument});

    auto response = socket->read();

    if (response.size() == 1 && response.at(0) == "OK")
    {
        return 0;
    }

    LOGERROR("Failed to stop "<< m_args.m_argument);
    return 1;
}
