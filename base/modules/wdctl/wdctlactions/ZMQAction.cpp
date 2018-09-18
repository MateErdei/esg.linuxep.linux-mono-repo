/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ZMQAction.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>

wdctl::wdctlactions::ZMQAction::ZMQAction(const wdctl::wdctlarguments::Arguments& args)
        : Action(args),m_context(Common::ZeroMQWrapper::createContext())
{
}

Common::ZeroMQWrapper::ISocketRequesterPtr wdctl::wdctlactions::ZMQAction::connectToWatchdog()
{
    Common::ZeroMQWrapper::ISocketRequesterPtr socket = m_context->getRequester();
    socket->setConnectionTimeout(1000); // 1 second timeout on connection to a unix socket
    socket->setTimeout(20000); // 20 second timeout on receiving data
    socket->connect(Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress());
    return socket;
}


Common::ZeroMQWrapper::IReadable::data_t wdctl::wdctlactions::ZMQAction::doOperationToWatchdog(const Common::ZeroMQWrapper::IWritable::data_t& arguments)
{
    try
    {
        Common::ZeroMQWrapper::ISocketRequesterPtr socket = connectToWatchdog();

        socket->write(arguments);

        return socket->read();
    }
    catch (const Common::ZeroMQWrapper::IIPCTimeoutException& ex)
    {
        return {std::string("Timeout out connecting to watchdog: ") + ex.what()};
    }
    catch (const Common::ZeroMQWrapper::IIPCException& ex)
    {
        return {std::string("IPC error: ") + ex.what()};
    }
}

bool wdctl::wdctlactions::ZMQAction::isSuccessful(const Common::ZeroMQWrapper::IReadable::data_t& response)
{
    return (response.size() == 1 && response.at(0) == "OK");
}

