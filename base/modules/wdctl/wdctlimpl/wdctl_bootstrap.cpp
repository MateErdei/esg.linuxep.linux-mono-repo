/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "wdctl_bootstrap.h"
#include "Logger.h"

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>

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
    std::string work_out_install_directory()
    {
        // Check if we have an environment variable telling us the installation location
        char* SOPHOS_INSTALL = secure_getenv("SOPHOS_INSTALL");
        if (SOPHOS_INSTALL != nullptr)
        {
            return SOPHOS_INSTALL;
        }
        // If we don't have environment variable, assume we were started in the installation directory
        char path[PATH_MAX];

//        ssize_t ret = ::readlink("/proc/self/exe", path, PATH_MAX);

        char* cwd = getcwd(path,PATH_MAX);
        if (cwd != nullptr)
        {
            return cwd;
        }
        // If we can't get the cwd then use a fixed string.
        return "/opt/sophos-spl";
    }
}

int wdctl_bootstrap::main(const StringVector& args)
{
    log4cplus::initialize();



    log4cplus::BasicConfigurator config;
    config.configure();

    m_args.parseArguments(args);

    GL_WDCTL_LOGGER = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("wdctl"));
    LOGINFO(m_args.m_command<<" "<<m_args.m_argument);

    log4cplus::Logger::shutdown();

    return 0;
}
