/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "wdctl_bootstrap.h"
#include "Logger.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>

using namespace wdctl::wdctlimpl;

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

int wdctl_bootstrap::main(const StringVector& args)
{
    log4cplus::initialize();

    log4cplus::BasicConfigurator config;
    config.configure();

    m_args.parseArguments(args);
    LOGINFO(m_args.m_command<<" "<<m_args.m_argument);

    log4cplus::Logger::shutdown();

    return 0;
}
