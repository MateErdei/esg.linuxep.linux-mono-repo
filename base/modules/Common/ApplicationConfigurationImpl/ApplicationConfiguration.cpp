//
// Created by pair on 29/06/18.
//

#include "ApplicationConfiguration.h"
namespace Common
{
    namespace ApplicationConfigurationImpl
    {

        std::string ApplicationConfiguration::getData(const std::string &key) const
        {
            return m_configurationData.at(key);
        }

        void ApplicationConfiguration::setConfigurationData(const std::map<std::string, std::string> &m_configurationData)
        {
            ApplicationConfiguration::m_configurationData = m_configurationData;
        }

        ApplicationConfiguration::ApplicationConfiguration()
        {
            m_configurationData[Common::ApplicationConfiguration::SOPHOS_INSTALL] = "/opt/sophos-spl";
        }

        void ApplicationConfiguration::setData(const std::string& key, const std::string& data)
        {
            m_configurationData[key] = data;
        }
    }


    namespace ApplicationConfiguration
    {

        IApplicationConfiguration& applicationConfiguration()
        {
            static Common::ApplicationConfigurationImpl::ApplicationConfiguration applicationConfiguration;
            return applicationConfiguration;
        }
    }
}

