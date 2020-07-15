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
#include <sys/mount.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystem/IFileSystemException.h>

namespace CommsComponent
{
    const std::string SOPHOS_DIRNAME = "sophos-spl-comms";

    void CommsConfigurator::applyChildSecurityPolicy()
    {
        setupLoggingFiles();
        Common::SecurityUtils::chrootAndDropPrivileges(m_childUser.userName, m_childUser.userGroup, m_sophosInstall);
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                    Common::ApplicationConfiguration::SOPHOS_INSTALL,"/");
    }


    void CommsConfigurator::applyParentSecurityPolicy()
    {
        Common::SecurityUtils::dropPrivileges(m_parentUser.userName, m_parentUser.userGroup);
    }


    void CommsConfigurator::applyParentInit()
    {
        umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
        m_logSetup.reset(new Common::Logging::FileLoggingSetup("parent",true)); //fixme, name
    }

    void CommsConfigurator::applyChildInit()
    {
        umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner

        m_logSetup.reset(new Common::Logging::FileLoggingSetup("child",true)); //fixme, name

        //ToDo LINUXDAR-1954
        // bind mount dirs = {"/lib","/usr/lib"};
        // bind mount file /etc/resolv.cof", "/etc/hosts" "/etc/ssl/ca-certificate.crt"};        //mount bind a file mount -o ro myfile destdir/myfile

        
    }

    CommsConfigurator::CommsConfigurator(const std::string &newRoot, UserConf childUser,
                                         UserConf parentUser)
            : m_chrootDir(newRoot), m_childUser(std::move(childUser)), m_parentUser(std::move(parentUser))
    {
        m_sophosInstall = Common::FileSystem::join(newRoot, SOPHOS_DIRNAME);
    }

    void CommsConfigurator::setupLoggingFiles()
    {
        try
        {
            std::string oldSophosInstall = Common::ApplicationConfiguration::applicationConfiguration().getData(
                    Common::ApplicationConfiguration::SOPHOS_INSTALL);
            std::vector<Path> loggingDirectories = {"base/etc/", "logs/base"};
            for (auto &dirpath : loggingDirectories)
            {
                Common::FileSystem::fileSystem()->makedirs(Common::FileSystem::join(m_sophosInstall, dirpath));
            }
            std::stringstream logName;

            Path loggerConfRelativePath = "base/etc/logger.conf";
            auto loggerConfSrc = Common::FileSystem::join(oldSophosInstall, loggerConfRelativePath);
            auto loggerConfDst = Common::FileSystem::join(m_sophosInstall, loggerConfRelativePath);
            Common::FileSystem::fileSystem()->copyFileAndSetPermissions(loggerConfSrc, loggerConfDst, 440,
                                                                        m_childUser.userName, m_childUser.userGroup);

            // fixme: is it necessary... could we just use the applicationconfiguration setData? 
            if (setenv(Common::ApplicationConfiguration::SOPHOS_INSTALL.c_str(), Common::FileSystem::join(
                    "/", SOPHOS_DIRNAME).c_str(), 1) == -1)
            {
                perror("Failed to reset SOPHOS_INSTAL environment: ");
            }
        }
        catch (const std::exception &ex)
        {
            std::cout << "Failed to configure logging " << ex.what() << std::endl;
        }
    }
}
