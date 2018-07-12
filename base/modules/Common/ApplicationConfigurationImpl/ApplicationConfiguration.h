//
// Created by pair on 29/06/18.
//

#ifndef EVEREST_BASE_APPLICATIONCONFIGURATION_H
#define EVEREST_BASE_APPLICATIONCONFIGURATION_H

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



#endif //EVEREST_BASE_APPLICATIONCONFIGURATION_H
