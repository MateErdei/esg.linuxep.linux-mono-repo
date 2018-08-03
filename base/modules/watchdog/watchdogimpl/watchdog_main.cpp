/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "watchdog_main.h"

#include "Logger.h"
#include "PluginProxy.h"
#include "SignalHandler.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Threads/NotifyPipe.h>

#include <cstdlib>

#include <unistd.h>
#include <sys/select.h>

#ifndef PATH_MAX
# define PATH_MAX 2048
#endif

using namespace watchdog::watchdogimpl;

/**
 * Static method called from watchdog executable
 * @param argc
 * @param argv
 * @return
 */
int watchdog_main::main(int argc, char **argv)
{
    static_cast<void>(argv); // unused
    if(argc > 1)
    {
        LOGERROR("Error, invalid command line arguments. Usage: watchdog");
        return 2;
    }

    watchdog_main m;
    return m.run();
}

namespace
{
    std::string work_out_install_directory()
    {
        // Check if we have an environment variable telling us the installation location
        char* SOPHOS_INSTALL = secure_getenv("SOPHOS_INSTALL");
        if (SOPHOS_INSTALL != nullptr)
        {
            return SOPHOS_INSTALL;
        }
        // If we don't have environment variable, assume we were started in the installation directory
        char pwd[PATH_MAX];
        char* cwd = getcwd(pwd,PATH_MAX);
        if (cwd != nullptr)
        {
            return cwd;
        }
        // If we can't get the cwd then use a fixed string.
        return "/opt/sophos-spl";
    }

    int addFD(fd_set* fds, int fd, int maxfd)
    {
        FD_SET(fd,fds); //NOLINT
        return std::max(maxfd,fd);
    }
}




int watchdog_main::run()
{
    SignalHandler signalHandler;

    std::string installDir = work_out_install_directory();
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL,
            installDir
            );

    PluginInfoVector pluginConfigs = read_plugin_configs();

    std::vector<watchdog::watchdogimpl::PluginProxy> proxies;

    for (auto& info : pluginConfigs)
    {
        proxies.emplace_back(info);
    }

    for (auto& proxy : proxies)
    {
        proxy.start();
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
        for (auto& proxy : proxies)
        {
            proxy.checkForExit();
        }
        timeout.tv_sec = 10;
        for (auto& proxy : proxies)
        {
            proxy.checkForExit();
            time_t waitPeriod = proxy.startIfRequired();
            timeout.tv_sec = std::min(waitPeriod, timeout.tv_sec);
        }

        timeout.tv_sec = std::max(timeout.tv_sec, static_cast<time_t>(1)); // Ensure we wait at least 1 second
        LOGDEBUG("timeout = "<<timeout.tv_sec);
    }

    LOGINFO("Stopping processes");
    for (auto& proxy : proxies)
    {
        proxy.stop();
    }

    return 0;
}

PluginInfoVector watchdog_main::read_plugin_configs()
{
    return Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();
}
