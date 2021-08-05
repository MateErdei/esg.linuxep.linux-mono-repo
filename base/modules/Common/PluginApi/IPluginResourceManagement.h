/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IBaseServiceApi.h"
#include "IEventVisitorCallback.h"
#include "IRawDataPublisher.h"
#include "ISubscriber.h"

#include <memory>
#include <string>

namespace Common
{
    namespace PluginApi
    {
        /**
         * IPluginResourceManagement is responsible for keeping threads that are needed by the IPluginAPI,
         * IRawDataPublisher and ISubscriber. Hence the IPluginResourceManagement must be constructed before them which
         * is trivial, as IPluginResourceManagement creates the other objects.
         *
         * @attention It must be destroyed only after the other objects that were constructed are already destroyed.
         *
         * It also work as a factory for the other classes that can be created by the create methods.
         *
         * On creation, the IPluginResourceManagement will setup the ipc channels that the plugin and sensors need to
         * operate correctly.
         *
         * @note It is safe to use the IPluginResourceManagement in any thread, but it is not safe to use it after fork
         * in the child process.
         */
        class IPluginResourceManagement
        {
        public:
            virtual ~IPluginResourceManagement() = default;

            /**
             * Creates an instance of the IPluginApi. It set it up with two ipc channels:
             *  * One that allows it to send requests to the Management Agent.
             *  * Another that allows mainly the Management Agent to send request to the Plugin which serviced via the
             * IPluginCallback. It also register the plugin with the Management Agent in order to allow it to operate.
             *
             * @param pluginName: Should be unique to the Management Agent. It will reject registration of the seccond
             * plugin presenting the same name.
             * @param pluginCallback: Instance of an IPluginCallback that will be used to serve requests from Management
             * Agent.
             * @return IPluginApi
             *
             * @note It throws if it can not successfully create and register a plugin.
             */
            virtual std::unique_ptr<IBaseServiceApi> createPluginAPI(
                const std::string& pluginName,
                std::shared_ptr<IPluginCallbackApi> pluginCallback) = 0;
        };

        std::unique_ptr<IPluginResourceManagement> createPluginResourceManagement();
    } // namespace PluginApi
} // namespace Common
