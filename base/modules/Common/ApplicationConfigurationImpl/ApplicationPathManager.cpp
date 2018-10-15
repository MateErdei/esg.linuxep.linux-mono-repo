/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/FileSystem/IFileSystem.h"
#include "ApplicationPathManager.h"
#include "IApplicationConfiguration.h"

namespace Common
{
    namespace ApplicationConfigurationImpl
    {

        std::string ApplicationPathManager::getPluginSocketAddress(const std::string &pluginName) const
        {
            return socketPath("plugins/" + pluginName + ".ipc");
        }

        std::string ApplicationPathManager::getManagementAgentSocketAddress() const
        {
            return socketPath("mcs_agent.ipc");
        }

        std::string ApplicationPathManager::getWatchdogSocketAddress() const
        {
            return socketPath("watchdog.ipc");
        }

        std::string ApplicationPathManager::sophosInstall() const
        {
            return Common::ApplicationConfiguration::applicationConfiguration().getData(Common::ApplicationConfiguration::SOPHOS_INSTALL);
        }

        std::string ApplicationPathManager::socketPath(const std::string &relative) const
        {
            try
            {
                // Allow an override with the relative path
                return Common::ApplicationConfiguration::applicationConfiguration().getData(relative);
            }
            catch (const std::out_of_range& )
            {
                // No override, so return the constructed value.
                // If the exception is a performance problem, then provide a getData that doesn't throw
                return "ipc://" + sophosInstall() + "/var/ipc/" + relative;
            }
        }

        std::string ApplicationPathManager::getPublisherDataChannelAddress() const
        {
            return socketPath("publisherdatachannel.ipc");
        }

        std::string ApplicationPathManager::getSubscriberDataChannelAddress() const
        {
            return socketPath("subscriberdatachannel.ipc");
        }

        std::string ApplicationPathManager::getPluginRegistryPath() const
        {
            return Common::FileSystem::join(sophosInstall(),"base/pluginRegistry/");
        }

        std::string ApplicationPathManager::getVersigPath() const
        {
            std::string versigPath = Common::FileSystem::join(sophosInstall(), "base/update/versig");
            if ( Common::FileSystem::fileSystem()->isFile(versigPath))
            {
                return versigPath;
            }
            auto envVersigPATH = ::secure_getenv("VERSIGPATH");
            if ( envVersigPATH == nullptr)
            {
                return versigPath;
            }
            return envVersigPATH;
        }

        std::string ApplicationPathManager::getMcsPolicyFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/policy");
        }

        std::string ApplicationPathManager::getMcsActionFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/action");
        }

        std::string ApplicationPathManager::getMcsStatusFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/status");
        }

        std::string ApplicationPathManager::getMcsEventFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/event");
        }

        std::string ApplicationPathManager::getManagementAgentStatusCacheFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/mcs/status/cache");
        }

        std::string ApplicationPathManager::getTempPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "tmp");
        }

        std::string ApplicationPathManager::getLocalWarehouseRepository() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/cache/primarywarehouse");
        }

        std::string ApplicationPathManager::getLocalDistributionRepository() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/cache/primary");
        }

        std::string ApplicationPathManager::getLocalUninstallSymLinkPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/var/installedproducts");
        }

        std::string ApplicationPathManager::getUpdateCertificatesPath() const
        {
            return Common::FileSystem::join(sophosInstall(), "base/update/certs");
        }

        std::string ApplicationPathManager::getBaseLogDirectory() const
        {
            return Common::FileSystem::join(sophosInstall(), "logs", "base");
        }

        std::string ApplicationPathManager::getBaseSophossplLogDirectory() const
        {
            return Common::FileSystem::join(getBaseLogDirectory(), "sophosspl");
        }

        std::string ApplicationPathManager::getUpdateCacheCertificateFilePath() const
        {
            return Common::FileSystem::join(getUpdateCertificatesPath(), "cache_certificates.crt");
        }


        std::string ApplicationPathManager::getSulDownloaderConfigFilePath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "config.json");
        }

        std::string ApplicationPathManager::getSulDownloaderReportGeneratedFilePath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "report.json");
        }

        std::string ApplicationPathManager::getSulDownloaderLockFilePath() const
        {
            return Common::FileSystem::join(sophosInstall(), "var/lock/suldownloader.pid");
        }

        std::string ApplicationPathManager::getSavedEnvironmentProxyFilePath() const
        {
            return Common::FileSystem::join(getSulDownloaderReportPath(), "savedproxy.config");
        }

    }


    namespace ApplicationConfiguration
    {
        std::unique_ptr<IApplicationPathManager>& instance()
        {
            static std::unique_ptr<IApplicationPathManager>  pointer = std::unique_ptr<IApplicationPathManager>(new ApplicationConfigurationImpl::ApplicationPathManager());
            return pointer;
        }

        IApplicationPathManager& applicationPathManager()
        {
            return *instance();
        }
        /** Use only for test */
        void replaceApplicationPathManager( std::unique_ptr<IApplicationPathManager>  applicationPathManager)
        {
            instance().reset(applicationPathManager.release()); // NOLINT
        }
        void restoreApplicationPathManager()
        {
            instance().reset(new ApplicationConfigurationImpl::ApplicationPathManager());
        }

    }

}