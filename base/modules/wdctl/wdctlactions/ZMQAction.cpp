/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ZMQAction.h"
#include "watchdog/watchdogimpl/Watchdog.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>

namespace
{
    constexpr char watchdogNotRunning[] = "Watchdog is not running";
}

wdctl::wdctlactions::ZMQAction::ZMQAction(const wdctl::wdctlarguments::Arguments& args) :
    Action(args),
    m_context(Common::ZMQWrapperApi::createContext())
{
}

Common::ZeroMQWrapper::ISocketRequesterPtr wdctl::wdctlactions::ZMQAction::connectToWatchdog()
{
    Common::ZeroMQWrapper::ISocketRequesterPtr socket = m_context->getRequester();
    socket->setConnectionTimeout(1000); // 1 second timeout on connection to a unix socket
    socket->setTimeout(20000);          // 20 second timeout on receiving data
    socket->connect(Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress());
    return socket;
}

Common::ZeroMQWrapper::IReadable::data_t wdctl::wdctlactions::ZMQAction::doOperationToWatchdog(
    const Common::ZeroMQWrapper::IWritable::data_t& arguments)
{
    for (std::string systemctlPath : { "/bin/systemctl", "/usr/sbin/systemctl" })
    {
        if (Common::FileSystem::fileSystem()->isFile(systemctlPath))
        {
            std::string systemCommand = systemctlPath + " status sophos-spl > /dev/null";
            if (system(systemCommand.c_str()) != 0)
            {
                return { std::string("Watchdog is not running") };
            }
            break;
        }
    }
    // if we do not find systemctlPath we then assume it might exist in another place
    // and allow the command to go through

    try
    {
        Common::ZeroMQWrapper::ISocketRequesterPtr socket = connectToWatchdog();

        socket->write(arguments);

        return socket->read();
    }
    catch (const Common::ZeroMQWrapper::IIPCTimeoutException& ex)
    {
        return { std::string("Timeout out connecting to watchdog: ") + ex.what() };
    }
    catch (const Common::ZeroMQWrapper::IIPCException& ex)
    {
        return { std::string("IPC error: ") + ex.what() };
    }
}

bool wdctl::wdctlactions::ZMQAction::isSuccessful(const Common::ZeroMQWrapper::IReadable::data_t& response)
{
    return (response.size() == 1 && response.at(0) == watchdog::watchdogimpl::watchdogReturnsOk);
}

bool wdctl::wdctlactions::ZMQAction::isSuccessfulOrWatchdogIsNotRunning(const Common::ZeroMQWrapper::IReadable::data_t& response)
{
    if (response.size() == 1)
    {
        std::string responseString = response.at(0);
        return (responseString == watchdog::watchdogimpl::watchdogReturnsOk || responseString  == watchdogNotRunning);
    }
    else
    {
        return false;
    }
}

