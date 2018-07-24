/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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

            char * installpath = secure_getenv("SOPHOS_INSTALL");
            std::string installDir = installpath != nullptr ? installpath : "/opt/sophos-spl";
            m_configurationData[Common::ApplicationConfiguration::SOPHOS_INSTALL] = installDir;
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

