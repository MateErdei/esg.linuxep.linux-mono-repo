/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Configurator.h"
#include "Logger.h"
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/UtilityImpl/SophosSplUsers.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <sstream>
#include <sys/stat.h>

namespace CommsComponent
{
    void CommsConfigurator::applyChildSecurityPolicy()
    {
        sophos::SophosSplUsers sophosSplUsers;
        std::string chrootDir("/opt/sophos-spl/var/");
        applyChildSecurityPolicy({sophosSplUsers.splNetworkUser(), sophosSplUsers.splNetworkGroup()}, chrootDir);
    }

    void CommsConfigurator::applyChildSecurityPolicy(const std::string &chrootdir)
    {
        sophos::SophosSplUsers sophosSplUsers;
        CommsConfigurator::applyChildSecurityPolicy({sophosSplUsers.splNetworkUser(), sophosSplUsers.splNetworkGroup()},
                                                    chrootdir);
    }

    void CommsConfigurator::applyChildSecurityPolicy(const std::pair<std::string, std::string> &lowPrivUser,
                                                     const std::string & /*chrootdir*/)
    {
        //LINUXDAR-1954 change to add chroot policy
        //Common::SecurityUtils::chrootAndDropPrivileges(user, group, chrootdir);
        Common::SecurityUtils::dropPrivileges(lowPrivUser.first, lowPrivUser.second);
    }

    void CommsConfigurator::applyParentSecurityPolicy()
    {
        sophos::SophosSplUsers sophosSplUsers;
        applyParentSecurityPolicy({sophosSplUsers.splLocalUser(), sophosSplUsers.splLocalGroup()});
    }

    void CommsConfigurator::applyParentSecurityPolicy(const std::pair<std::string, std::string> &lowPrivUser)
    {
        Common::SecurityUtils::dropPrivileges(lowPrivUser.first, lowPrivUser.second);
    }

    void CommsConfigurator::applyParentInit()
    {
        umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
    }

    void CommsConfigurator::applyChildInit()
    {
        //everything is read & execute
        umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
        //set & create chroot dir
        //set permissions to chroot path
        //copy /etc/resolv.conf
        //copy /etc/hosts
        //create /usr/
        //mount --bound /lib & /usr/lib to chroot & chroot/usr
        setupLoggingFiles("/opt/sophos-spl/");
    }

    void setupLoggingFiles(const std::string &logBaseDir)
    {
        try
        {
            std::vector<Path> loggingDirectories = {"base/etc/", "logs/base"};
            for (auto &dirpath : loggingDirectories)
            {
                Common::FileSystem::fileSystem()->makedirs(Common::FileSystem::join(logBaseDir, dirpath));
            }
            std::stringstream logName;

            Path loggerConfRelativePath = "base/etc/logger.conf";
            auto loggerConfSrc = Common::FileSystem::join(
                    Common::ApplicationConfiguration::applicationConfiguration().getData(
                            Common::ApplicationConfiguration::SOPHOS_INSTALL), loggerConfRelativePath);
            auto loggerConfDst = Common::FileSystem::join(logBaseDir, loggerConfRelativePath);
            Common::FileSystem::fileSystem()->copyFile(loggerConfSrc, loggerConfDst);
            setenv(Common::ApplicationConfiguration::SOPHOS_INSTALL.c_str(), logBaseDir.c_str(), 1);
            //Common::Logging::FileLoggingSetup loggerSetup("commsnetwork", true);
        }
        catch (const std::exception &ex)
        {
            std::cout << "Failed to configure logging " << ex.what() << std::endl;
        }
    }
}
