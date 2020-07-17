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
    void NullConfigurator::applyChildInit()
    {
        m_console.reset(new Common::Logging::ConsoleLoggingSetup());
    }

    void NullConfigurator::applyParentInit()
    {
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
        mountDependenciesReadOnly(m_childUser, m_listOfDependencyPairs);
    }

    void CommsConfigurator::applyChildInit()
    {
        //umask(S_IXUSR | S_IXGRP | S_IWGRP | S_IRWXO); // Read and write for the owner and read for group 027

        m_logSetup.reset(new Common::Logging::FileLoggingSetup(m_childUser.logName));
    }

    CommsConfigurator::CommsConfigurator(const std::string& newRoot, UserConf childUser, UserConf parentUser,
                                         std::vector<ReadOnlyMount> dependenciesToMount)
            : m_chrootDir(newRoot), m_childUser(std::move(childUser)), m_parentUser(std::move(parentUser)),
              m_listOfDependencyPairs(std::move(dependenciesToMount)) {}

    void CommsConfigurator::setupLoggingFiles()
    {
        try
        {
            std::string oldSophosInstall = Common::ApplicationConfiguration::applicationConfiguration().getData(
                    Common::ApplicationConfiguration::SOPHOS_INSTALL);
            std::vector<Path> loggingDirectories = {"base", "base/etc/", "logs", "logs/base"};
            for (auto& dirpath : loggingDirectories)
            {
                std::string path = Common::FileSystem::join(m_chrootDir, dirpath);
                Common::FileSystem::fileSystem()->makedirs(path);
                Common::FileSystem::filePermissions()->chown(path, m_childUser.userName, m_childUser.userGroup);
            }
            std::stringstream logName;

            Path loggerConfRelativePath = "base/etc/logger.conf";
            auto loggerConfSrc = Common::FileSystem::join(oldSophosInstall, loggerConfRelativePath);
            auto loggerConfDst = Common::FileSystem::join(m_chrootDir, loggerConfRelativePath);
            Common::FileSystem::fileSystem()->copyFileAndSetPermissions(loggerConfSrc, loggerConfDst, 440,
                                                                        m_childUser.userName, m_childUser.userGroup);

        }
        catch (const std::exception& ex)
        {
            std::stringstream errMsg;
            errMsg << "Failed to configure logging " << ex.what();
            perror(errMsg.str().c_str());
        }
    }

    void CommsConfigurator::mountDependenciesReadOnly(UserConf userConf, std::vector<ReadOnlyMount> dependencies)
    {
        auto fs = Common::FileSystem::fileSystem();
        auto ifperms = Common::FileSystem::FilePermissionsImpl();

        for (const auto& target : dependencies)
        {
            auto sourcePath = target.first;
            auto targetPath = Common::FileSystem::join(m_chrootDir, target.second);

            //attempting to mount to an existing file will overwrite
            if (fs->exists(targetPath)&&!Common::SecurityUtils::isFreeMountLocation(targetPath))
            {
                std::stringstream errorMessage;
                if (Common::SecurityUtils::isAlreadyMounted(targetPath))
                {
                    LOGINFO("Source '" << sourcePath << "' is already mounted on '" << targetPath);
                    continue;
                }
                //this could be anything do not mount ontop
                LOGERROR("file without the expected content found in the mount location");
                exit(EXIT_FAILURE);
            }

            if (fs->isFile(sourcePath))
            {
                auto dirpath = Common::FileSystem::dirName(targetPath);
                std::cout << "dirname " << dirpath << std::endl;
                fs->makedirs(dirpath);
                fs->writeFile(targetPath, "");
                ifperms.chown(targetPath, userConf.userName, userConf.userGroup); //test-spl-user //test-spl-grp
                ifperms.chmod(dirpath, 0755);
            }

            else if (fs->isDirectory(sourcePath))
            {
                fs->makedirs(targetPath);
                ifperms.chown(targetPath, userConf.userName, userConf.userGroup);
                ifperms.chmod(targetPath, 0700);
            }
            Common::SecurityUtils::bindMountReadOnly(sourcePath, targetPath);
        }
    }
}
