/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Configurator.h"
#include "Logger.h"
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <CommsComponent/CommsComponentUtils.h>

#include <sstream>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/UtilityImpl/StringUtils.h>


namespace
{
    std::string getLogsBackUpPath(const std::string& logName)
    {
        std::stringstream backupName;
        backupName << logName << "_backup";

        return Common::FileSystem::join(Common::ApplicationConfiguration::applicationPathManager().getTempPath(), backupName.str());
    }

    std::vector<std::string> getMountedEntitiesFromDependencies(const std::string& chrootDir, const std::vector<CommsComponent::ReadOnlyMount>& listOfDependencies)
    {
        std::vector<std::string> mountedPaths;
        for (auto& mountOption : listOfDependencies)
        {
            std::string pathMounted = Common::UtilityImpl::StringUtils::startswith(mountOption.second, "/")
                                      ? mountOption.second.substr(1) : mountOption.second;
            mountedPaths.emplace_back(Common::FileSystem::join(chrootDir, mountOption.second));
        }
        return mountedPaths;
    }

    bool isSafeToPurgeChroot(const std::vector<std::string>& listOfMountedPaths, std::ostream& out)
    {
        for (auto &mountedPath : listOfMountedPaths)
        {
            auto isOurFreeMountLocation = Common::SecurityUtils::isFreeMountLocation(mountedPath, out);
            if (!isOurFreeMountLocation)
            {
                return false;
            }
        }
        return true;
    }

    void cleanMountedPaths(const std::vector<std::string>& listOfMountedPaths, std::ostream& out)
    {
        for (auto& mountedPath : listOfMountedPaths)
        {
            out <<"Unmount path: " << mountedPath;
            Common::SecurityUtils::unMount(mountedPath, out);
        }
    }

}
namespace CommsComponent
{
    void NullConfigurator::applyChildSecurityPolicy()
    {
        m_console.reset(new Common::Logging::ConsoleLoggingSetup());
    }

    void NullConfigurator::applyParentSecurityPolicy()
    {
        m_console.reset(new Common::Logging::ConsoleLoggingSetup());
    }

    void CommsConfigurator::applyChildSecurityPolicy()
    {
        std::stringstream output;
        try
        {
            backupLogsAndRemoveChrootDir(output);
            //create fresh chroot dir
            Common::FileSystem::fileSystem()->makedirs(m_chrootDir);
            Common::FileSystem::filePermissions()->chown(m_chrootDir, m_childUser.userName, m_childUser.userGroup);

            setupLoggingFiles();
            restoreLogs();

            if (mountDependenciesReadOnly(m_childUser, m_listOfDependencyPairs, m_chrootDir, output)
                == CommsConfigurator::MountOperation::MountFailed)
            {
                throw Common::SecurityUtils::FatalSecuritySetupFailureException("Failed to configure the mount points");
            }
            Common::SecurityUtils::chrootAndDropPrivileges(m_childUser.userName, m_childUser.userGroup, m_chrootDir,
                                                           output);
        }
        catch (Common::SecurityUtils::FatalSecuritySetupFailureException& ex)
        {
            std::cout << output.str() << std::endl;
            std::cerr << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            std::cout << output.str() << std::endl;
            std::cerr << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        Common::ApplicationConfiguration::applicationConfiguration().setData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL, "/");
        Path logPath = Common::FileSystem::join("/logs", m_childUser.logName + ".log");
        m_logSetup.reset(new Common::Logging::FileLoggingSetup(logPath));
        LOGINFO(output.str());
    }


    void CommsConfigurator::applyParentSecurityPolicy()
    {
        std::stringstream output;
        try
        {
            Common::SecurityUtils::dropPrivileges(m_parentUser.userName, m_parentUser.userGroup, output);
        }
        catch (Common::SecurityUtils::FatalSecuritySetupFailureException& ex)
        {
            std::cout << output.str() << std::endl;
            std::cerr << ex.what() << std::endl;
            exit(EXIT_FAILURE);
        }

        m_logSetup.reset(new Common::Logging::FileLoggingSetup(m_parentUser.logName, true));
        LOGINFO(output.str());
    }

    CommsConfigurator::CommsConfigurator(std::string newRoot, UserConf childUser, UserConf parentUser,
                                         std::vector<ReadOnlyMount> dependenciesToMount)
            : m_chrootDir(std::move(newRoot)), m_childUser(std::move(childUser)), m_parentUser(std::move(parentUser)),
              m_listOfDependencyPairs(std::move(dependenciesToMount)) {}

    void CommsConfigurator::setupLoggingFiles()
    {
        try
        {
            std::string oldSophosInstall = Common::ApplicationConfiguration::applicationConfiguration().getData(
                    Common::ApplicationConfiguration::SOPHOS_INSTALL);
            std::vector<Path> loggingDirectories = {"", "base", "base/etc/", "logs"};
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
            Common::FileSystem::fileSystem()->copyFileAndSetPermissions(loggerConfSrc, loggerConfDst, 0440,
                                                                        m_childUser.userName, m_childUser.userGroup);

        }
        catch (const std::exception& ex)
        {
            std::stringstream errMsg;
            errMsg << "Failed to configure logging " << ex.what();
            perror(errMsg.str().c_str());
        }
    }


    CommsConfigurator::MountOperation CommsConfigurator::mountDependenciesReadOnly(
            const CommsComponent::UserConf& userConf,
            const std::vector<ReadOnlyMount>& listOfDependencyPairs, const std::string& chrootDir, std::ostream& out)
    {
        auto fs = Common::FileSystem::fileSystem();
        auto ifperms = Common::FileSystem::FilePermissionsImpl();

        for (const auto& target : listOfDependencyPairs)
        {
            auto sourcePath = target.first;
            auto targetRelPath = target.second;

            //remove "/" if at the start of the path
            if (Common::UtilityImpl::StringUtils::startswith(targetRelPath, "/"))
            {
                targetRelPath = targetRelPath.substr(1);
            }

            auto targetPath = Common::FileSystem::join(chrootDir, targetRelPath);

            //attempting to mount to an existing file will overwrite
            if (fs->exists(targetPath)&&!Common::SecurityUtils::isFreeMountLocation(targetPath, out))
            {
                if (Common::SecurityUtils::isAlreadyMounted(targetPath, out))
                {
                    out << "Configure source '" << sourcePath << "' is already mounted on '" << targetPath << "\n";
                    continue;
                }
                //this could be anything do not mount ontop
                out << "File without the expected content found in the mount location: " << targetPath << "\n";
                return MountOperation::MountFailed;
            }

            if (fs->isFile(sourcePath))
            {
                makeDirsAndSetPermissions(chrootDir, Common::FileSystem::dirName(targetRelPath), userConf.userName,
                                          userConf.userGroup, 0755);
                fs->writeFile(targetPath, "");
                ifperms.chown(targetPath, userConf.userName, userConf.userGroup); //test-spl-user //test-spl-grp
            }
            else if (fs->isDirectory(sourcePath))
            {
                CommsComponent::makeDirsAndSetPermissions(chrootDir, targetRelPath, userConf.userName,
                                                          userConf.userGroup, 0700);
            }
            if (!Common::SecurityUtils::bindMountReadOnly(sourcePath, targetPath, out))
            {
                return MountOperation::MountFailed;
            };
        }
        return MountOperation::MountSucceeded;
    }


    //throws handle
    std::vector<ReadOnlyMount> CommsConfigurator::getListOfDependenciesToMount()
    {
        std::vector<ReadOnlyMount> allPossiblePaths{
                {"/lib",             "lib"},
                {"/usr/lib",         "usr/lib"},
                {"/etc/hosts",       "etc/hosts"},
                {"/etc/resolv.conf", "etc/resolv.conf"},
                {"/usr/lib64",       "usr/lib64"}
        };

        std::vector<ReadOnlyMount>  validDeps; 
        for( auto & dep: allPossiblePaths)
        {
            if( Common::FileSystem::fileSystem()->exists(dep.first))
            {
                validDeps.emplace_back(dep); 
            }
        }

        //add default certifcate store path
        auto certStorePath = CommsComponent::getCertificateStorePath();
        if (certStorePath.has_value())
        {
            validDeps.emplace_back(std::make_pair(certStorePath.value(), certStorePath->substr(1)));
        }

        //add mcs certs folder
        std::string sophosInstall = Common::ApplicationConfiguration::applicationConfiguration().getData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL);
        auto mcsCertPathSource = Common::FileSystem::join(sophosInstall, "base/mcs/certs");
        if (Common::FileSystem::fileSystem()->isDirectory(mcsCertPathSource))
        {
            validDeps.emplace_back(std::make_pair(mcsCertPathSource, "base/mcs/certs"));
        }

        return validDeps;
    }


    std::vector<std::string> CommsConfigurator::getListOfMountedEntities(const std::string& chrootDir)
    {
        return getMountedEntitiesFromDependencies(chrootDir, getListOfDependenciesToMount());
    }

    void CommsConfigurator::cleanDefaultMountedPaths(const std::string& chrootDir)
    {
        std::stringstream out;
        auto listOfMountedPaths = getListOfMountedEntities(chrootDir);
        cleanMountedPaths(listOfMountedPaths, out);
        LOGINFO(out.str());
    }

    std::string CommsConfigurator::chrootPathForSSPL(const std::string& ssplRootDir)
    {
        return Common::FileSystem::join(ssplRootDir, "var/sophos-spl-comms");
    }

    void CommsConfigurator::backupLogsAndRemoveChrootDir(std::ostream& out)
    {
        if (Common::FileSystem::fileSystem()->exists(m_chrootDir))
        {
            auto listOfMountedPaths = getMountedEntitiesFromDependencies(m_chrootDir, m_listOfDependencyPairs);
            cleanMountedPaths(listOfMountedPaths, out);

            if (!isSafeToPurgeChroot(listOfMountedPaths, out))
            {
                std::cout << out.rdbuf() << std::endl;
                exit(EXIT_FAILURE);
            }
            backupLogs();
            Common::FileSystem::fileSystem()->removeFileOrDirectory(m_chrootDir);
        }
    }

    //Restore logs back into chroot path
    void CommsConfigurator::restoreLogs()
    {
        auto fs = Common::FileSystem::fileSystem();
        auto logsDir = Common::FileSystem::join(m_chrootDir, "logs");
        auto backup = getLogsBackUpPath(m_childUser.logName);
        if(!fs->exists(logsDir) || !fs->exists(backup))
        {
            return;
        }

        if ( fs->isDirectory(backup))
        {
            for (auto& logFile : fs->listFiles(backup))
            {
                auto destLogFile = Common::FileSystem::join(logsDir, Common::FileSystem::basename(logFile));
                fs->copyFileAndSetPermissions(logFile, destLogFile, 0600, m_childUser.userName, m_childUser.userGroup);
            }
            fs->removeFileOrDirectory(backup);
        }
    }

    //backup logs in sophos temp
    void CommsConfigurator::backupLogs()
    {
        auto fs = Common::FileSystem::fileSystem();
        auto logsDir = Common::FileSystem::join(m_chrootDir,"logs");
        if (fs->exists(logsDir))
        {
            auto listOfFiles = fs->listFiles(logsDir);
            if (!listOfFiles.empty())
            {
                fs->moveFile(logsDir, getLogsBackUpPath(m_childUser.logName));
            }
        }
    }
}
