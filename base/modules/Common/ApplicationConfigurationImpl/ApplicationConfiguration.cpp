/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ApplicationConfiguration.h"
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
            char* installpath = secure_getenv("SOPHOS_INSTALL");
            std::string installDir = installpath != nullptr ? installpath : "/opt/sophos-spl";
            m_configurationData[Common::ApplicationConfiguration::SOPHOS_INSTALL] = installDir;
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
