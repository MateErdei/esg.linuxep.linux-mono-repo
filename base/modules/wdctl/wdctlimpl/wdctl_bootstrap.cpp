/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "wdctl_bootstrap.h"
#include "Logger.h"
#include "LoggingSetup.h"

#include <wdctl/wdctlactions/CopyPlugin.h>
#include <wdctl/wdctlactions/RemoveAction.h>
#include <wdctl/wdctlactions/StartAction.h>
#include <wdctl/wdctlactions/StopAction.h>

#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <cstdlib>
#include <csignal>

#include <unistd.h>
#include <sys/select.h>

#ifndef PATH_MAX
# define PATH_MAX 2048
#endif

using namespace wdctl::wdctlimpl;

log4cplus::Logger GL_WDCTL_LOGGER; //NOLINT

int wdctl_bootstrap::main(int argc, char **argv)
{
    LoggingSetup logging;

    if (argc != 3)
    {
        LOGERROR("Error: Wrong number of arguments expected 2");
        return 2;
    }

    StringVector args = convertArgv(static_cast<unsigned int>(argc), argv);
    return main(args);
}

StringVector wdctl_bootstrap::convertArgv(unsigned int argc, char **argv)
{
    StringVector result;
    result.reserve(argc);
    for (unsigned int i=0; i<argc; ++i)
    {
        result.emplace_back(argv[i]);
    }
    return result;
}
namespace
{
    Path make_absolute(const Path& path)
    {
        return Common::FileSystem::fileSystem()->make_absolute(path);
    }

    Path get_executable_path()
    {
        char path[PATH_MAX];
        ssize_t ret = ::readlink("/proc/self/exe", path, PATH_MAX); // $SOPHOS_INSTALL/bin/wdctl.x
        if (ret > 0)
        {
            return make_absolute(path);
        }
        return Path();
    }

    Path work_out_install_directory()
    {
        // Check if we have an environment variable telling us the installation location
        char* SOPHOS_INSTALL = secure_getenv("SOPHOS_INSTALL");
        if (SOPHOS_INSTALL != nullptr)
        {
            return SOPHOS_INSTALL;
        }

        // If we don't have the environment variable, see if we can work out from the executable
        Path exe = get_executable_path();
        if (!exe.empty())
        {
            Path bindir = Common::FileSystem::dirName(exe); // $SOPHOS_INSTALL/bin
            return Common::FileSystem::dirName(bindir); // installdir $SOPHOS_INSTALL
        }

        // If we can't get the cwd then use a fixed string.
        return "/opt/sophos-spl";
    }

}

int wdctl_bootstrap::main(const StringVector& args)
{
    const Path SOPHOS_INSTALL = work_out_install_directory();

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL,
            SOPHOS_INSTALL
    );



    m_args.parseArguments(args);

    LOGINFO(m_args.m_command<<" "<<m_args.m_argument);

    std::unique_ptr<wdctl::wdctlactions::Action> action;

    std::string command = m_args.m_command;

    if (command == "copyPluginRegistration")
    {
        action.reset(new wdctl::wdctlactions::CopyPlugin(m_args));
    }
    else if (command == "stop")
    {
        action.reset(new wdctl::wdctlactions::StopAction(m_args));
    }
    else if (command == "start")
    {
        action.reset(new wdctl::wdctlactions::StartAction(m_args));
    }
    else if (command == "removePluginRegistration")
    {
        action.reset(new wdctl::wdctlactions::RemoveAction(m_args));
    }
    else
    {
        LOGERROR("Unknown command: " << m_args.m_command);
        return 10;
    }
    return action->run();
}
