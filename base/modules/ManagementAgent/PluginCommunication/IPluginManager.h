//
// Created by pair on 05/07/18.
//

#ifndef EVEREST_BASE_IPLUGINMANAGER_H
#define EVEREST_BASE_IPLUGINMANAGER_H

#include <string>
#include "IPluginProxy.h"

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginManager
    {
        virtual void registerPlugin(std::string pluginName) = 0;
        virtual std::shared_ptr<PluginCommunication::IPluginProxy> getPlugin(std::string pluginName) = 0;
        virtual void removePlugin(std::string pluginName) = 0;
    };
}
}

#endif //EVEREST_BASE_IPLUGINMANAGER_H

