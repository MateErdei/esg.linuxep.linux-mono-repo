/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LogSetup.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/Logging/FileLoggingSetup.h"
#include "Common/Logging/LoggerConfig.h"
#include <Common/Logging/SophosLoggerMacros.h>
#include "../common/config.h"

#include <log4cplus/logger.h>

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
    auto chroot = pluginInstall + "/chroot";
    auto chrootLinkDir = chroot + pluginInstall;
    auto linkLocation = chrootLinkDir + "/log/sophos_threat_detector";

    // check if link is present
    bool linkAlreadyPresent = false;
    int linkCreatedErrno = 0;
    struct stat statbuf{};
    int ret = lstat(linkLocation.c_str(), &statbuf);
    if (ret == 0)
    {
        linkAlreadyPresent = true;
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

    auto logfilepath =  pluginInstall + "/log/sophos_threat_detector/sophos_threat_detector.log";

    Common::Logging::FileLoggingSetup::setupFileLoggingWithPath(logfilepath);
    Common::Logging::applyGeneralConfig(PLUGIN_NAME);

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
