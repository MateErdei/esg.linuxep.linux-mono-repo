/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Configurator.h"
#include "Logger.h"
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/Logging/FileLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <CommsComponent/CommsComponentUtils.h>

#include <sstream>
#include <sys/stat.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/UtilityImpl/StringUtils.h>

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
        mountDependenciesReadOnly(m_childUser);
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
    }

    CommsConfigurator::CommsConfigurator(const std::string& newRoot, UserConf childUser, UserConf parentUser,
                                         std::vector<ReadOnlyMount> dependenciesToMount)
            : m_chrootDir(newRoot), m_childUser(std::move(childUser)), m_parentUser(std::move(parentUser)),
              m_listOfDependencyPairs(std::move(dependenciesToMount)) {}

    CommsConfigurator::CommsConfigurator(const std::string& newRoot, UserConf childUser, UserConf parentUser)

            : m_chrootDir(newRoot), m_childUser(std::move(childUser)), m_parentUser(std::move(parentUser)),
              m_listOfDependencyPairs(getListOfDependenciesToMount()) {}

    void CommsConfigurator::setupLoggingFiles()
    {
        try
        {
            std::string oldSophosInstall = Common::ApplicationConfiguration::applicationConfiguration().getData(
                    Common::ApplicationConfiguration::SOPHOS_INSTALL);
            std::vector<Path> loggingDirectories = {"","base", "base/etc/", "logs", "logs/base"};
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


    void CommsConfigurator::mountDependenciesReadOnly(CommsComponent::UserConf userConf)
    {
        auto fs = Common::FileSystem::fileSystem();
        auto ifperms = Common::FileSystem::FilePermissionsImpl();

        for (const auto& target : m_listOfDependencyPairs)
        {
            auto sourcePath = target.first;
            auto targetRelPath = target.second;

            //remove "/" if at the start of the path
            if (Common::UtilityImpl::StringUtils::startswith(targetRelPath, "/"))
            {
                targetRelPath = targetRelPath.substr(1);
            }

            auto targetPath = Common::FileSystem::join(m_chrootDir, targetRelPath);

            //attempting to mount to an existing file will overwrite
            if (fs->exists(targetPath)&&!Common::SecurityUtils::isFreeMountLocation(targetPath))
            {
                std::stringstream errorMessage;
                if (Common::SecurityUtils::isAlreadyMounted(targetPath))
                {
                    errorMessage << "Configure source '" << sourcePath << "' is already mounted on '" << targetPath;
                    perror(errorMessage.str().c_str());
                    continue;
                }
                //this could be anything do not mount ontop
                perror("file without the expected content found in the mount location");
                exit(EXIT_FAILURE);
            }

            if (fs->isFile(sourcePath))
            {
                makeDirsAndSetPermissions(m_chrootDir, Common::FileSystem::dirName(targetRelPath), userConf.userName,
                                          userConf.userGroup, 0755);
                assert(fs->exists(Common::FileSystem::dirName(targetPath)));
                fs->writeFile(targetPath, "");
                ifperms.chown(targetPath, userConf.userName, userConf.userGroup); //test-spl-user //test-spl-grp
            }
            else if (fs->isDirectory(sourcePath))
            {
                CommsComponent::makeDirsAndSetPermissions(m_chrootDir, targetRelPath, userConf.userName,
                                                          userConf.userGroup, 0700);
                assert(fs->exists(targetPath));
            }
            Common::SecurityUtils::bindMountReadOnly(sourcePath, targetPath);
        }
    }


    //throws handle
    std::vector<ReadOnlyMount> CommsConfigurator::getListOfDependenciesToMount()
    {
        std::vector<ReadOnlyMount> deps{
                {"/lib",             "lib"},
                {"/usr/lib",         "usr/lib"},
                {"/etc/hosts",       "etc/hosts"},
                {"/etc/resolv.conf", "etc/resolv.conf"}
        };

        //add default certifcate store path
        auto certStorePath = CommsComponent::getCertificateStorePath();
        if (certStorePath.has_value())
        {
            deps.emplace_back(std::make_pair(certStorePath.value(), certStorePath->substr(1)));
        }

        //add mcs certs folder
        std::string sophosInstall = Common::ApplicationConfiguration::applicationConfiguration().getData(
                Common::ApplicationConfiguration::SOPHOS_INSTALL);
        auto mcsCertPathSource = Common::FileSystem::join(sophosInstall, "base/mcs/certs");
        if (Common::FileSystem::fileSystem()->isDirectory(mcsCertPathSource))
        {
            deps.emplace_back(std::make_pair(mcsCertPathSource, "base/mcs/certs"));
        }

        return deps;
    }


    std::vector<std::string> CommsConfigurator::getListOfMountedEntities(const std::string& chrootDir)
    { 
        std::vector<std::string> mountedPaths;    
        for( auto & mountOption : getListOfDependenciesToMount())
        {
            std::string pathMounted = Common::UtilityImpl::StringUtils::startswith(mountOption.second, "/") ? mountOption.second.substr(1) : mountOption.second; 
            mountedPaths.emplace_back( Common::FileSystem::join( chrootDir, mountOption.second )); 
        }
        return mountedPaths; 
    }
    void CommsConfigurator::cleanDefaultMountedPaths(const std::string & chrootDir)
    {
        auto listOfMountedPaths = getListOfMountedEntities(chrootDir); 
        for( auto & mountedPath : listOfMountedPaths)
        {
            LOGINFO("Unmount path: " << mountedPath); 
            Common::SecurityUtils::unMount(mountedPath);
        }
    }

    std::string CommsConfigurator::chrootPathForSSPL(const std::string & ssplRootDir)
    {
        return Common::FileSystem::join(ssplRootDir, "var", GL_SOPHOS_DIRNAME); 
    }

}
