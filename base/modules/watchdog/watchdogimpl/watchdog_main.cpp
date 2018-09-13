/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include "watchdog_main.h"

#include "Logger.h"
#include "Watchdog.h"
#include "LoggingSetup.h"

#include <unistd.h>

#ifndef PATH_MAX
# define PATH_MAX 2048
#endif

using namespace watchdog::watchdogimpl;
log4cplus::Logger GL_WATCHDOG_LOGGER;
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
        char* cwd = ::getcwd(pwd, PATH_MAX);
        if (cwd != nullptr)
        {
            return cwd;
        }
        // If we can't get the cwd then use a fixed string.
        return "/opt/sophos-spl";
    }
}

/**
 * Static method called from watchdog executable
 * @param argc
 * @param argv
 * @return
 */
int watchdog_main::main(int argc, char **argv)
{
    static_cast<void>(argv); // unused

    LOGINFO("Starting WatchDog Version 999");

    if(argc > 1)
    {
        LOGERROR("Error, invalid command line arguments. Usage: watchdog");
        return 2;
    }
    watchdog::watchdogimpl::LoggingSetup loggingSetup;
    std::string installDir = work_out_install_directory();
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL,
            installDir
    );

    Watchdog m;
    return m.run();
}
