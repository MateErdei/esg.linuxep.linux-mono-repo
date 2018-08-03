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

#include <cstdlib>

#include <unistd.h>
#include <sys/select.h>

using namespace watchdog::watchdogimpl;

namespace
{
    int addFD(fd_set* fds, int fd, int maxfd)
    {
        FD_SET(fd,fds); //NOLINT
        return std::max(maxfd,fd);
    }
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

    fd_set read_fds;
    int max_fd = 0;
    FD_ZERO(&read_fds);

    max_fd = addFD(&read_fds,signalHandler.subprocessExitFileDescriptor(),max_fd);
    max_fd = addFD(&read_fds,signalHandler.terminationFileDescriptor(),max_fd);

    struct timespec timeout{ .tv_sec=10 ,.tv_nsec=0 };

    while (keepRunning)
    {
        // TODO: LINUXEP-5920 Handle wdctl commands

        fd_set read_temp = read_fds;
        LOGDEBUG("Calling pselect at "<<::time(nullptr));
        int active = ::pselect(max_fd+1,
                               &read_temp,
                               nullptr,
                               nullptr,
                               &timeout,
                               nullptr);
        LOGDEBUG("Returned from pselect: "<<active<<" at "<<::time(nullptr));
        if (active < 0)
        {
            LOGERROR("pselect returned error: "<<errno);
            continue;
        }
        if (active > 0)
        {
            if (FD_ISSET(signalHandler.terminationFileDescriptor(), &read_temp)) //NOLINT
            {
                LOGWARN("Sophos watchdog exiting");
                signalHandler.clearTerminationPipe();
                keepRunning = false;
                continue;
            }
            if (FD_ISSET(signalHandler.subprocessExitFileDescriptor(), &read_temp)) //NOLINT
            {
                LOGERROR("Child process died");
                signalHandler.clearSubProcessExitPipe();
            }
        }
        timeout.tv_sec = 10;
        for (auto& proxy : m_pluginProxies)
        {
            proxy.checkForExit();
            time_t waitPeriod = proxy.startIfRequired();
            timeout.tv_sec = std::min(waitPeriod, timeout.tv_sec);
        }

        timeout.tv_sec = std::max(timeout.tv_sec, static_cast<time_t>(1)); // Ensure we wait at least 1 second
        LOGDEBUG("timeout = "<<timeout.tv_sec);
    }

    LOGINFO("Stopping processes");
    for (auto& proxy : m_pluginProxies)
    {
        proxy.stop();
    }

    return 0;
}

PluginInfoVector Watchdog::read_plugin_configs()
{
    return Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();
}
