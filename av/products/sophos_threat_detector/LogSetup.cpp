/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include <Common/Logging/LoggingSetup.h>
#include <Common/Logging/SophosLoggerMacros.h>
#include "../common/config.h"

#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>

#include <iterator>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static log4cplus::Logger& getLogSetupLogger()
{
    static log4cplus::Logger STATIC_LOGGER = Common::Logging::getInstance("LogSetup");
    return STATIC_LOGGER;
}

#define LOGERROR(x) LOG4CPLUS_ERROR(getLogSetupLogger(), x)  // NOLINT

namespace
{
    std::string linkLocationCreator(const std::string& pluginInstall)
    {
        auto chroot = pluginInstall + "/chroot";
        auto chrootLinkDir = chroot + pluginInstall;
        return chrootLinkDir + "/log/sophos_threat_detector";
    }

    std::tuple<bool, int> ensureLoggingSymlinkPresent(const std::string& pluginInstall, const std::string& linkLocation)
    {
        int linkCreatedErrno = 0;
        struct stat statbuf
        {
        };
        int ret = lstat(linkLocation.c_str(), &statbuf);
        if (ret == 0)
        {
            return {true, 0};
        }
        else
        {
            // Create the symlink
            size_t depth = std::count(pluginInstall.begin(), pluginInstall.end(), '/');
            std::ostringstream os;
            std::fill_n(std::ostream_iterator<std::string>(os), depth, "../");
            os << "../log";
            auto linkDestination = os.str();
            ret = ::symlink(linkDestination.c_str(), linkLocation.c_str());
            if (ret != 0)
            {
                linkCreatedErrno = errno;
            }
        }
        return {false, linkCreatedErrno};
    }

    void setupFileLoggingWithPath(const std::string& logfilepath)
    {
        log4cplus::initialize();

        log4cplus::tstring datePattern;
        const long maxFileSize = 10 * 1024 * 1024;
        const int maxBackupIndex = 10;
        const bool immediateFlush = true;
        const bool createDirs = true;
        log4cplus::SharedAppenderPtr appender(
            new log4cplus::RollingFileAppender(logfilepath, maxFileSize, maxBackupIndex, immediateFlush, createDirs));
        Common::Logging::LoggingSetup::applyDefaultPattern(appender);
        log4cplus::Logger::getRoot().addAppender(appender);

        // Log error messages to stderr
        log4cplus::SharedAppenderPtr stderr_appender(new log4cplus::ConsoleAppender(true));
        stderr_appender->setThreshold(log4cplus::ERROR_LOG_LEVEL);
        Common::Logging::LoggingSetup::applyPattern(stderr_appender, Common::Logging::LoggingSetup::GL_CONSOLE_PATTERN);
        log4cplus::Logger::getRoot().addAppender(stderr_appender);
    }

    void applyGeneralConfig(const std::string& logbase)
    {
        Common::Logging::applyGeneralConfig(logbase);
    }
}

LogSetup::LogSetup()
{
    auto sophosInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();

    // Ensure symlink is present
    // PLUGIN_INSTALL="${SOPHOS_INSTALL}/plugins/${PLUGIN_NAME}"
    // CHROOT="${PLUGIN_INSTALL}/chroot"
    // CHROOT_LINK_DIR="${CHROOT}/${PLUGIN_INSTALL}"
    // BACKLINK="$(echo "${PLUGIN_INSTALL%/}" | sed -e "s:[^/]*:..:g;s:^/::")"
    // ln -snf "${BACKLINK}/log" "${CHROOT_LINK_DIR}/log/sophos_threat_detector"

    auto pluginInstall = sophosInstall + "/plugins/av";
    auto linkLocation = linkLocationCreator(pluginInstall); // also needed for reporting

    // check if link is present
    auto linkCreatedResult = ensureLoggingSymlinkPresent(pluginInstall, linkLocation);
    auto linkAlreadyPresent = std::get<0>(linkCreatedResult);
    auto linkCreatedErrno = std::get<1>(linkCreatedResult);

    auto logfilepath = pluginInstall + "/log/sophos_threat_detector/sophos_threat_detector.log";

    setupFileLoggingWithPath(logfilepath);
    applyGeneralConfig(PLUGIN_NAME); // Sets the threshold for the root logger

    // Log after the logging has been setup
    if (!linkAlreadyPresent)
    {
        if (linkCreatedErrno == 0)
        {
            LOGERROR("Create symlink for logs at " << linkLocation);
        }
        else
        {
            LOGERROR("Failed to create symlink for logs at " << linkLocation << ": " << linkCreatedErrno);
        }
    }
}

LogSetup::~LogSetup()
{
    log4cplus::Logger::shutdown();
}
