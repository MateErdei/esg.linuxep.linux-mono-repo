/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ApplicationConfiguration.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

namespace
{

    Path workOutInstallDirectory()
    {
        // Check if we have an environment variable telling us the installation location
        char* SOPHOS_INSTALL = secure_getenv("SOPHOS_INSTALL");
        if (SOPHOS_INSTALL != nullptr)
        {
            return SOPHOS_INSTALL;
        }
        // Use local instantiation of file system to avoid upsetting mockFilesyse
        Common::FileSystem::FileSystemImpl fileSystem;
        // If we don't have the environment variable, see if we can work out from the executable
        Path exe = fileSystem.readlink("/proc/self/exe"); // either $SOPHOS_INSTALL/base/bin/X,
                                                                                 // $SOPHOS_INSTALL/bin/X or
                                                                                 // $SOPHOS_INSTALL/plugins/PluginName/X
        if (!exe.empty())
        {
            std::string installTipDir = "sophos-spl"; // All custom installation directories have sophos-spl at the tip
                                                      // this is added by the thin-installer
            size_t pos = exe.find(installTipDir);
            if (pos !=std::string::npos) {
               return exe.substr(0, pos+installTipDir.size());
            }
        }

        // If we can't get the cwd then use a fixed string.
        return Common::ApplicationConfigurationImpl::DefaultInstallLocation;
    }
} // namespace


namespace Common
{
    namespace ApplicationConfigurationImpl
    {
        std::string ApplicationConfiguration::getData(const std::string& key) const
        {
            return m_configurationData.at(key);
        }

        void ApplicationConfiguration::setConfigurationData(const configuration_data_t& m_configurationData)
        {
            ApplicationConfiguration::m_configurationData = m_configurationData;
        }

        ApplicationConfiguration::ApplicationConfiguration()
        {
            m_configurationData[Common::ApplicationConfiguration::SOPHOS_INSTALL] = workOutInstallDirectory();
        }

        void ApplicationConfiguration::setData(const std::string& key, const std::string& data)
        {
            m_configurationData[key] = data;
        }
    } // namespace ApplicationConfigurationImpl

    namespace ApplicationConfiguration
    {
        IApplicationConfiguration& applicationConfiguration()
        {
            static Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
            return applicationConfiguration;
        }
    } // namespace ApplicationConfiguration
} // namespace Common
