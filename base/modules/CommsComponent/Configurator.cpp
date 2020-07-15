/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Configurator.h"
#include "Logger.h"
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <sstream>
#include <sys/stat.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>

namespace CommsComponent
{
    void NullConfigurator::applyChildInit(){
        m_console.reset(new Common::Logging::ConsoleLoggingSetup());
    }
    
    void NullConfigurator::applyParentInit(){
        m_console.reset(new Common::Logging::ConsoleLoggingSetup());
    }

    void CommsConfigurator::applyChildSecurityPolicy()
    {
        setupLoggingFiles();
        Common::SecurityUtils::chrootAndDropPrivileges(m_childUser.userName, m_childUser.userGroup, m_chrootDir);
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/");
    }


    void CommsConfigurator::applyParentSecurityPolicy()
    {
        Common::SecurityUtils::dropPrivileges(m_parentUser.userName, m_parentUser.userGroup);
    }


    void CommsConfigurator::applyParentInit()
    {
        //umask(S_IRWXG | S_IRWXO | S_IXUSR); // Read and write for the owner
        m_logSetup.reset(new Common::Logging::FileLoggingSetup(m_parentUser.logName, true));
    }

    void CommsConfigurator::applyChildInit()
    {
        //umask(S_IXUSR | S_IXGRP | S_IWGRP | S_IRWXO); // Read and write for the owner and read for group 027

        m_logSetup.reset(new Common::Logging::FileLoggingSetup(m_childUser.logName));

        //ToDo LINUXDAR-1954
        // bind mount dirs = {"/lib","/usr/lib"};
        // bind mount file /etc/resolv.cof", "/etc/hosts" "/etc/ssl/ca-certificate.crt"};        //mount bind a file mount -o ro myfile destdir/myfile


    }

    CommsConfigurator::CommsConfigurator(const std::string &newRoot, UserConf childUser,
                                         UserConf parentUser)
            : m_chrootDir(newRoot), m_childUser(std::move(childUser)), m_parentUser(std::move(parentUser))
    {
    }

    void CommsConfigurator::setupLoggingFiles()
    {
        try
        {
            std::string oldSophosInstall = Common::ApplicationConfiguration::applicationConfiguration().getData(
                    Common::ApplicationConfiguration::SOPHOS_INSTALL);
            std::vector<Path> loggingDirectories = {"base", "base/etc/", "logs", "logs/base"};
            for (auto &dirpath : loggingDirectories)
            {
                std::string path = Common::FileSystem::join(m_chrootDir, dirpath); 
                Common::FileSystem::fileSystem()->makedirs(path);
                Common::FileSystem::filePermissions()->chown(path,m_childUser.userName, m_childUser.userGroup); 
            }
            std::stringstream logName;

            Path loggerConfRelativePath = "base/etc/logger.conf";
            auto loggerConfSrc = Common::FileSystem::join(oldSophosInstall, loggerConfRelativePath);
            auto loggerConfDst = Common::FileSystem::join(m_chrootDir, loggerConfRelativePath);
            Common::FileSystem::fileSystem()->copyFileAndSetPermissions(loggerConfSrc, loggerConfDst, 440,
                                                                        m_childUser.userName, m_childUser.userGroup);

            std::cout << "coppied config file correctly " << std::endl;
        }
        catch (const std::exception &ex)
        {
            std::cout << "Failed to configure logging " << ex.what() << std::endl;
        }
    }
}
