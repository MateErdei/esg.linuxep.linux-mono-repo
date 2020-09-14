/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "watchdog_main.h"

#include "Logger.h"
#include "Watchdog.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <CommsComponent/Configurator.h>
#include <Common/Logging/FileLoggingSetup.h>

#include <unistd.h>
#include <sys/stat.h>
#include <Common/UtilityImpl/ConfigException.h>

#ifndef PATH_MAX
#    define PATH_MAX 2048
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
} // namespace

/**
 * Static method called from watchdog executable
 * @param argc
 * @param argv
 * @return
 */
int watchdog_main::main(int argc, char** argv)
{
    umask(S_IRWXG | S_IRWXO); // Read and write for the owner
    static_cast<void>(argv); // unused

    std::string installDir = work_out_install_directory();
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, installDir);
    Common::Logging::FileLoggingSetup logSetup("watchdog", false);
    if (argc > 1)
    {
        LOGERROR("Error, invalid command line arguments. Usage: watchdog");
        return 2;
    }
    try
    {        
        Watchdog m;
        return m.initialiseAndRun();
    }
    catch ( Common::UtilityImpl::ConfigException & ex)
    {
        LOGFATAL( ex.what());
        return 1;
    }

}
