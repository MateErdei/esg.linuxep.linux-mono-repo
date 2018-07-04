//
// Created by pair on 29/06/18.
//

#ifndef EVEREST_BASE_IAPPLICATIONCONFIGURATION_H
#define EVEREST_BASE_IAPPLICATIONCONFIGURATION_H
#include <string>
#include <memory>
namespace Common
{
    namespace ApplicationConfiguration
    {
        static const std::string SOPHOS_INSTALL = "SOPHOS_INSTALL";

        class IApplicationConfiguration
        {
        public:
            virtual ~IApplicationConfiguration() = default;
            virtual std::string getData(const std::string & key) const = 0;
        };

        IApplicationConfiguration& applicationConfiguration();
    }
}


#endif //EVEREST_BASE_IAPPLICATIONCONFIGURATION_H
