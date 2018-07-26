/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include <map>

namespace Common
{
    namespace ApplicationConfigurationImpl
    {
        class ApplicationConfiguration : public virtual Common::ApplicationConfiguration::IApplicationConfiguration
        {
        public:
            ApplicationConfiguration();
            std::string getData(const std::string & key) const override ;

            void setConfigurationData(const std::map<std::string, std::string> &m_configurationData);

        private:
            std::map<std::string, std::string> m_configurationData;


        };
    }
}




