//
// Created by pair on 28/06/18.
//

#ifndef EVEREST_BASE_IPLUGINCALLBACK_H
#define EVEREST_BASE_IPLUGINCALLBACK_H

#include <string>

namespace Common
{
    namespace PluginApi
    {

        class IPluginCallback
        {
        public:
            virtual ~IPluginCallback() = default;
            /// Receives the new policy and pass the text to the plugin.
            /// \param policyXml
            virtual void applyNewPolicy(const std::string &policyXml) = 0;

            virtual void doAction(const std::string &actionXml) = 0;

            virtual void shutdown() = 0;

            virtual void getStatus(std::string &statusXml, std::string &statusWithoutTimestampsXml) = 0;

            virtual std::string getTelemetry() = 0;
        };

    }
}
#endif //EVEREST_BASE_IPLUGINCALLBACK_H
