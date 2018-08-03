/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Watchdog.h"
#include "SignalHandler.h"
#include "PluginProxy.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Threads/NotifyPipe.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>

#include <cstdlib>

#include <unistd.h>
#include <sys/select.h>
#include <Common/ZeroMQWrapper/IPoller.h>

using namespace watchdog::watchdogimpl;

Watchdog::~Watchdog()
{
    m_socket.reset();
    m_context.reset();
}

int Watchdog::run()
{
    SignalHandler signalHandler;

    PluginInfoVector pluginConfigs = read_plugin_configs();

    m_pluginProxies.clear();
    m_pluginProxies.reserve(pluginConfigs.size());

    for (auto& info : pluginConfigs)
    {
        m_pluginProxies.emplace_back(info);
    }

    pluginConfigs.clear();

    for (auto& proxy : m_pluginProxies)
    {
        proxy.startIfRequired();
    }

    bool keepRunning = true;

    setupSocket();

    Common::ZeroMQWrapper::IPollerPtr poller = Common::ZeroMQWrapper::createPoller();

    Common::ZeroMQWrapper::IHasFDPtr subprocessFD = poller->addEntry(
            signalHandler.subprocessExitFileDescriptor(),
            Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN
            );
    Common::ZeroMQWrapper::IHasFDPtr terminationFD = poller->addEntry(
            signalHandler.terminationFileDescriptor(),
            Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN
            );

    poller->addEntry(
            *m_socket,
             Common::ZeroMQWrapper::IPoller::PollDirection::POLLIN
            );

    std::chrono::seconds timeout(10);

    while (keepRunning)
    {
        LOGDEBUG("Calling poller at "<<::time(nullptr));
        Common::ZeroMQWrapper::IPoller::poll_result_t active =
                poller->poll(std::chrono::milliseconds(timeout));
        LOGDEBUG("Returned from poller: "<<active.size()<<" at "<<::time(nullptr));

        for (auto& fd : active)
        {
            if (fd == terminationFD.get())
            {
                LOGWARN("Sophos watchdog exiting");
                signalHandler.clearTerminationPipe();
                keepRunning = false;
                continue;
            }
            if (fd == subprocessFD.get())
            {
                LOGERROR("Child process died");
                signalHandler.clearSubProcessExitPipe();
            }
            if (fd == m_socket.get())
            {
                handleSocketRequest();
            }
        }

        timeout = std::chrono::seconds(10);
        for (auto& proxy : m_pluginProxies)
        {
            proxy.checkForExit();
            auto waitPeriod = proxy.startIfRequired();
            timeout = std::min(waitPeriod, timeout);
        }

        timeout = std::max(timeout, std::chrono::seconds(1)); // Ensure we wait at least 1 second
        LOGDEBUG("timeout = "<<timeout.count());
    }

    LOGINFO("Stopping processes");
    for (auto& proxy : m_pluginProxies)
    {
        proxy.stop();
    }

    // Normal shutdown
    m_pluginProxies.clear();
    m_socket.reset();
    m_context.reset();

    return 0;
}

PluginInfoVector Watchdog::read_plugin_configs()
{
    return Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();
}

std::string Watchdog::getIPCPath()
{
    return Common::ApplicationConfiguration::applicationPathManager().getWatchdogSocketAddress();
}

void Watchdog::setupSocket()
{
    m_socket = m_context->getReplier();
    m_socket->listen(getIPCPath());
}

void Watchdog::handleSocketRequest()
{
    Common::ZeroMQWrapper::IReadable::data_t request = m_socket->read();
    LOGINFO("Command from IPC: "<<request.at(0));
    Common::ZeroMQWrapper::IWritable::data_t response;
    response.emplace_back("OK");
    m_socket->write(response);
}

Watchdog::Watchdog()
    : Watchdog(Common::ZeroMQWrapper::createContext())
{
}

Watchdog::Watchdog(Common::ZeroMQWrapper::IContextSharedPtr context)
    : m_context(std::move(context))
{
}
