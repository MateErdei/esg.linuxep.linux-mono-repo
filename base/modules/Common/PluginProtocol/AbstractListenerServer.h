// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/PluginProtocol/Protocol.h"
#include "Common/Reactor/ICallbackListener.h"
#include "Common/Reactor/IReactor.h"
#include "Common/ZeroMQWrapper/IReadWrite.h"
#include "Common/ZeroMQWrapper/IReadable.h"

#include <memory>

namespace Common::PluginProtocol
{
    using namespace PluginProtocol;

    class AbstractListenerServer : public virtual Common::Reactor::ICallbackListener,
                                   public virtual Common::Reactor::IShutdownListener
    {
    public:
        enum class ARMSHUTDOWNPOLICY
        {
            DONOTARM,
            HANDLESHUTDOWN
        };
        AbstractListenerServer(
            std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> ireadWrite,
            ARMSHUTDOWNPOLICY armshutdownpolicy);
        void start();
        void stop();
        void stopAndJoin();

    private:
        virtual DataMessage process(const DataMessage& request) const = 0;
        virtual void onShutdownRequested() = 0;

        void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request) override;
        void notifyShutdownRequested() override;
        std::unique_ptr<Common::ZeroMQWrapper::IReadWrite> m_ireadWrite;
        std::unique_ptr<Common::Reactor::IReactor> m_reactor;
    };
} // namespace Common::PluginProtocol
