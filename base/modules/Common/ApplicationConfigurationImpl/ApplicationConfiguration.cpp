/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ApplicationConfiguration.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

namespace
{
    // Below directories should exist in the sophos install path and have
    // correct permissions for both root and sophos-spl-user to query
    constexpr char pluginRegRelPath[] = "base/pluginRegistry";
    constexpr char baseBinRelPath[] = "base/bin";

    Path workOutInstallDirectory()
    {
        // Check if we have an environment variable telling us the installation location
        char* SOPHOS_INSTALL = secure_getenv("SOPHOS_INSTALL");
        if (SOPHOS_INSTALL != nullptr)
        {
            return SOPHOS_INSTALL;
        }
        // If we don't have the environment variable, see if we can work out from the executable
        Path exe = Common::FileSystem::fileSystem()->readlink("/proc/self/exe"); // either $SOPHOS_INSTALL/base/bin/X,
                                                                                 // $SOPHOS_INSTALL/bin/X or
                                                                                 // $SOPHOS_INSTALL/plugins/PluginName/X
        if (!exe.empty())
        {
            std::string baseDirName = Common::FileSystem::dirName(exe);
            while (!baseDirName.empty())
            {
                // Check if expected directories exist here
                std::string checkPath1 = Common::FileSystem::join(baseDirName, pluginRegRelPath);
                std::string checkPath2 = Common::FileSystem::join(baseDirName, baseBinRelPath);
                if (Common::FileSystem::fileSystem()->exists(checkPath1) &&
                    Common::FileSystem::fileSystem()->exists(checkPath2))
                {
                    return baseDirName;
                }
                baseDirName = Common::FileSystem::dirName(baseDirName);
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

        ApplicationConfiguration::ApplicationConfiguration()
        {
            m_configurationData[Common::ApplicationConfiguration::SOPHOS_INSTALL] = workOutInstallDirectory();
            m_configurationData[Common::ApplicationConfiguration::TELEMETRY_RESTORE_DIR] =
                    Common::FileSystem::join(m_configurationData[Common::ApplicationConfiguration::SOPHOS_INSTALL],
                            "base/telemetry/cache");
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
