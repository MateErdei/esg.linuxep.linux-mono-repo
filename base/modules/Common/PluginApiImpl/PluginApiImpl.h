//
// Created by pair on 02/07/18.
//

#ifndef EVEREST_BASE_PLUGINAPIIMPL_H
#define EVEREST_BASE_PLUGINAPIIMPL_H


#include "Common/PluginApi/IPluginCallback.h"
#include "Common/PluginApi/IPluginApi.h"
#include "PluginCallBackHandler.h"
#include "Common/ZeroMQWrapper/ISocketRequesterPtr.h"
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"

namespace Common
{
    namespace PluginApiImpl
    {

    class PluginApiImpl : public virtual Common::PluginApi::IPluginApi
        {
        public:

            PluginApiImpl(const std::string& pluginName, Common::ZeroMQWrapper::ISocketRequesterPtr socketRequester );
            ~PluginApiImpl();

            void setPluginCallback(std::shared_ptr<Common::PluginApi::IPluginCallback> pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr  );

            void sendEvent(const std::string& appId, const std::string& eventXml) const override;

            void changeStatus(const std::string& appId, const std::string& statusXml, const std::string& statusWithoutTimestampsXml) const override;

            std::string getPolicy(const std::string &appId) const override ;
        private:
            DataMessage getReply( const DataMessage & request) const;

            std::string m_pluginName;
            Common::ZeroMQWrapper::ISocketRequesterPtr  m_socket;
            std::unique_ptr<PluginCallBackHandler> m_pluginCallbackHandler;
    };
    }
}

#endif //EVEREST_BASE_PLUGINAPIIMPL_H
