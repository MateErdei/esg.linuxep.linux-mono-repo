/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINAPI_H
#define EVEREST_BASE_PLUGINAPI_H

#include "IPluginCallbackApi.h"
#include "ISensorDataCallback.h"
#include <memory>

namespace Common
{
    namespace PluginApi
    {

        /** Plugin in this product is related to executables that implement one or more functionalities expected by Management Console from an App.
         *
         *  @see https://wiki.sophos.net/display/SophosCloud/Endpoint+Management+Protocol
         *
         *  From the point of view of the Management Console an endpoint will have App installed ( for example, SAV, ALC, HBT, etc).
         *  Each App is required to be able to understand its Policy and provide feedback via Status.
         *  Some Apps also support Actions.
         *
         *  For the design of this product, the Plugins are managed by the Management Agent and their status, policies and actions go to and come from
         *  Management Console indirectly via this Management Agent.
         *
         *  The IPluginApi aims to simplify the interaction and facilitate the development of Plugins in the product, implementing
         *  all the ipc communication between the Plugin and the Agent and exposing a simple API for Plugins developers.
         *
         *  The Plugin is to be created by IPluginResourceManagement and it requires for its creation to be given also the IPluginCallback which defines the
         *  services that a Plugin must provide to the Management Agent.
         *
         *  The common case will be that there will be a one-to-one map between the Plugin and the Management Console App representation. But there are scenario
         *  that a Plugin may want to implement functionalities of two Apps or that two Plugins colaborate to implement a single App. For this reason,
         *  ::sendEvent and ::changeStatus and ::getPolicy require that the appId must be provide explicitly.
         *
         */
        class IBaseServiceApi
        {
        public:
            virtual ~IBaseServiceApi() = default;

            /**
             * Send an Event to Management Console via the Management Agent.
             *
             * The EventXml must conform to the protocol defined in https://wiki.sophos.net/display/SophosCloud/Endpoint+Management+Protocol
             * section  "Events & Status: Endpoint → Management"
             *
             * @param appId The App name as required by Management Console.
             * @param eventXml The content of the xml to be sent to Management Console.
             * @throw ApiException May throw if plugin fail to Contact ManagementAgent or if it rejects the call.
             */
            virtual void sendEvent(const std::string& appId, const std::string& eventXml) const  = 0;

            /**
             * Report the App Status to Management Console.
             *
             * Management Console require that status of Apps in an Endpoint be sent periodically and whenever it changes.
             *
             * The expected content of status for each App is available in the wiki (link provided).
             *
             * The EndPoint is requested to not send status to Management Console unless it has changed. To simplify the algorithm
             * that track if status has changed, Management Agent keep track of statusWithoutTimestampsXml and compare previous
             * content with any received one.
             *
             * Hence, a plugin may decide that instead of sending the content of the status twice, it will send as statusWithoutTimestampXml
             * a hash of the content of the status that does not change with timestamps and can be compared against future statusWithoutTimestampsXml
             * to define if the received status has changed or not.
             *
             * @param appId The App name as required by Management Console.
             * @param statusXml The content of the xml to be sent to Management Console.
             * @param statusWithoutTimestampsXml A representation of status that can be used reliably to identify that status has in effect changed.
             * @throw ApiException May throw if plugin fail to Contact ManagementAgent or if it rejects the call.
             */
            virtual void sendStatus(const std::string &appId, const std::string &statusXml,
                                    const std::string &statusWithoutTimestampsXml) const = 0;

            /**
             * Query the Management Agent for the current policy to be applied to the App identified by AppId.
             * @param appId The App name as required by Management Console.
             * @return Either the xmlContent of the policy or the 'translation of it'
             * @throw ApiException May throw if plugin fail to Contact ManagementAgent or if it rejects the call.
             * @todo add information about 'translating xml'
             */
            virtual std::string getPolicy(const std::string &appId) const = 0;

        };

        //std::string getLibraryVersion();
    }
}


#endif //EVEREST_BASE_PLUGINAPI_H
