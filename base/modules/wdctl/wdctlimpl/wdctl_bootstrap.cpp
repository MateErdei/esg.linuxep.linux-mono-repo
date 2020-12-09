/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "wdctl_bootstrap.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <sys/select.h>
#include <wdctl/wdctlactions/CopyPlugin.h>
#include <wdctl/wdctlactions/IsRunning.h>
#include <wdctl/wdctlactions/RemoveAction.h>
#include <wdctl/wdctlactions/StartAction.h>
#include <wdctl/wdctlactions/StopAction.h>

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

using namespace wdctl::wdctlimpl;

int wdctl_bootstrap::main(int argc, char** argv)
{
    if (argc != 3)
    {
        // calling wdctl with wrong number of arguments will happen by users calling, hence, console output is correct.
        std::cerr << "Error: Wrong number of arguments expected 2" << std::endl;
        return 2;
    }

    StringVector args = convertArgv(static_cast<unsigned int>(argc), argv);
    return main(args);
}

StringVector wdctl_bootstrap::convertArgv(unsigned int argc, char** argv)
{
    StringVector result;
    result.reserve(argc);
    for (unsigned int i = 0; i < argc; ++i)
    {
        result.emplace_back(argv[i]);
    }
    return result;
}

int wdctl_bootstrap::main(const StringVector& args)
{
    Common::Logging::FileLoggingSetup logSetup("wdctl", false);
    return main_afterLogConfigured(args);
}

int wdctl_bootstrap::main_afterLogConfigured(const StringVector& args, bool detectWatchdog)
{
    m_args.parseArguments(args);

    LOGINFO(m_args.m_command << " " << m_args.m_argument);

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
    else if (command == "isrunning")
    {
        action.reset(new wdctl::wdctlactions::IsRunning(m_args));
    }
    else
    {
        LOGERROR("Unknown command: " << m_args.m_command);
        return 10;
    }
    if (!detectWatchdog)
    {
        action->setSkipWatchdogDetection();
    }
    return action->run();
}
