/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "watchdog_main.h"

#include "Logger.h"
#include "PluginProxy.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Threads/NotifyPipe.h>

#include <cstdlib>
#include <csignal>

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
        return -1;
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

    Common::Threads::NotifyPipe GL_CHILD_DEATH_PIPE;
    Common::Threads::NotifyPipe GL_TERM_PIPE;

    void signal_handler(int signal)
    {
        if (signal == SIGCHLD)
        {
            GL_CHILD_DEATH_PIPE.notify();
        }
        else if (signal == SIGINT || signal == SIGTERM)
        {
            GL_TERM_PIPE.notify();
        }
    }

    void setSignalHandler()
    {
        struct sigaction signalBuf{0}; //NOLINT
        signalBuf.sa_handler = signal_handler;
        sigemptyset(&signalBuf.sa_mask);
        signalBuf.sa_flags = SA_NOCLDSTOP | SA_RESTART; //NOLINT
        ::sigaction(SIGCHLD, &signalBuf, nullptr);
        ::sigaction(SIGINT, &signalBuf, nullptr);
        ::sigaction(SIGTERM, &signalBuf, nullptr);
    }

    int addFD(fd_set* fds, int fd, int maxfd)
    {
        FD_SET(fd,fds); //NOLINT
        return std::max(maxfd,fd);
    }
}




int watchdog_main::run()
{
    setSignalHandler();

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
    fd_set write_fds;
    fd_set except_fds;
    int max_fd = 0;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_ZERO(&except_fds);

    max_fd = addFD(&read_fds,GL_CHILD_DEATH_PIPE.readFd(),max_fd);
    max_fd = addFD(&read_fds,GL_TERM_PIPE.readFd(),max_fd);

    struct timespec timeout{ .tv_sec=10 ,.tv_nsec=0 };

    while (keepRunning)
    {
        // TODO: LINUXEP-5920 Handle wdctl commands

        fd_set read_temp = read_fds;
        int active = pselect(max_fd+1, &read_temp, nullptr, nullptr,
                &timeout,
                nullptr);
        if (active == 0)
        {
            continue;
        }
        if (FD_ISSET(GL_TERM_PIPE.readFd(),&read_temp)) //NOLINT
        {
            while (GL_TERM_PIPE.notified())
            {
            }
            keepRunning = false;
            continue;
        }
        if (FD_ISSET(GL_CHILD_DEATH_PIPE.readFd(),&read_temp)) //NOLINT
        {
            while (GL_CHILD_DEATH_PIPE.notified())
            {
            }
            for (auto& proxy : proxies)
            {
                if (proxy.status() == Common::Process::ProcessStatus::FINISHED)
                {
                    int exitCode = proxy.exitCode();
                    if (exitCode != 0)
                    {
                        LOGERROR("Process died with "<<exitCode);
                    }
                    proxy.start();
                }
            }
        }
    }

    for (auto& proxy : proxies)
    {
        if (proxy.status() == Common::Process::ProcessStatus::RUNNING)
        {
            proxy.stop();
        }
    }

    return 0;
}

PluginInfoVector watchdog_main::read_plugin_configs()
{
    PluginInfoVector plugins = Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

    for (auto &plugin: plugins)
    {
        LOGINFO("plugin " << plugin.getPluginName() << " is at " << plugin.getExecutableFullPath());
    }
    return plugins;
}
