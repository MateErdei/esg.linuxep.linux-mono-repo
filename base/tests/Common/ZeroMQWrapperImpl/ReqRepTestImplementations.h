/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#ifndef ARTISANBUILD

#include <Common/ZeroMQWrapper/IReadable.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequesterPtr.h>
#include <Common/ZeroMQWrapper/ISocketReplierPtr.h>
#include <Common/ZeroMQWrapper/ISocketPublisher.h>
#include <Common/ZeroMQWrapper/ISocketSubscriber.h>
#include <Common/ZeroMQWrapperImpl/SocketSubscriberImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketRequesterImpl.h>
#include <Common/ZeroMQWrapperImpl/SocketReplierImpl.h>
#include <Common/ZeroMQWrapper/IIPCTimeoutException.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>

#include <iostream>
#include <cassert>
#include <thread>
#include <future>
#include <zmq.h>
#include <sys/types.h>
#include <unistd.h>

namespace ReqRepTest
{
    Common::ZeroMQWrapper::IContextSharedPtr createContext();

    using data_t = Common::ZeroMQWrapper::IReadable::data_t;

    /** uses the requester as implemented in PluginProxy::getReply and  Common::PluginApiImpl::BaseServiceAPI::getReply**/
    class Requester
    {
        Common::ZeroMQWrapper::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    public:
        explicit Requester(const std::string& serverAddress)
                :
                m_context(createContext())
        {
            m_requester = m_context->getRequester();
            m_requester->setTimeout(1000);
            m_requester->setConnectionTimeout(1000);
            m_requester->connect(serverAddress);
        }

        explicit Requester(const std::string& serverAddress, Common::ZeroMQWrapper::IContextSharedPtr context)
            :
                m_context(std::move(context))
        {
            m_requester = m_context->getRequester();
            m_requester->setTimeout(1000);
            m_requester->setConnectionTimeout(1000);
            m_requester->connect(serverAddress);
        }

        std::string sendReceive(const std::string& value)
        {
            m_requester->write(data_t{value});
            data_t answer = m_requester->read();
            return answer.at(0);
        }
        ~Requester() = default;
    };


    class Replier
    {
        Common::ZeroMQWrapper::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketReplierPtr m_replier;
    public:
        explicit Replier(const std::string& serverAddress, int timeout = 1000)
                :
                m_context(createContext())
        {
            m_replier = m_context->getReplier();
            m_replier->setTimeout(timeout);
            m_replier->setConnectionTimeout(timeout);
            m_replier->listen(serverAddress);
        }

        Replier(const std::string& serverAddress, Common::ZeroMQWrapper::IContextSharedPtr context, int timeout = 1000)
                :
                m_context(std::move(context))
        {
            m_replier = m_context->getReplier();
            m_replier->setTimeout(timeout);
            m_replier->setConnectionTimeout(timeout);
            m_replier->listen(serverAddress);
        }

        void serveRequest()
        {
            auto request = m_replier->read();
            m_replier->write(request);
        }
        ~Replier() = default;
    };

    class Unreliable
    {
    protected:
        Common::ZeroMQWrapper::IContextSharedPtr m_context;
        Common::ZeroMQWrapper::ISocketRequesterPtr m_requestKillChannel;

        void requestKill()
        {
            m_requestKillChannel->write(data_t{"killme"});
        }
        void notifyShutdown()
        {
            m_requestKillChannel->write(data_t{"close"});
        }
    public:
        explicit Unreliable( const std::string & killChannelAddress )
        {
            m_context = createContext();
            m_requestKillChannel = m_context->getRequester();
            m_requestKillChannel->connect(killChannelAddress);
        }
    };

    class UnreliableReplier : public Unreliable
    {
        Common::ZeroMQWrapper::ISocketReplierPtr m_replier;
    public:
        UnreliableReplier(const std::string& serverAddress, const std::string& killChannelAddress)
                : Unreliable(killChannelAddress)
        {
            m_replier = m_context->getReplier();
            m_replier->setTimeout(1000);
            m_replier->setConnectionTimeout(1000);
            m_replier->listen(serverAddress);
        }

        void serveRequest()
        {
            data_t request = m_replier->read();
            m_replier->write(data_t{"granted"});
            notifyShutdown();
        }
        void breakAfterReceiveMessage()
        {
            data_t request = m_replier->read();
            requestKill();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        void servePartialReply()
        {
            auto * socketHolder = dynamic_cast<Common::ZeroMQWrapperImpl::SocketImpl*> (m_replier.get());
            assert(socketHolder);
            data_t request = m_replier->read();

            void* socket = socketHolder->skt();
            int rc;
            std::string firstElement = "firstElement";
            // send the first element but do not send the rest
            rc = zmq_send(socket, firstElement.data(), firstElement.size(), ZMQ_SNDMORE);
            assert(rc >= 0);
            static_cast<void>(rc);
            requestKill();
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    };

    class UnreliableRequester : public Unreliable
    {
        Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    public:
        UnreliableRequester( const std::string & serverAddress, const std::string & killChannelAddress )
                : Unreliable( killChannelAddress)
        {
            m_requester = m_context->getRequester();
            m_requester->setTimeout(1000);
            m_requester->setConnectionTimeout(1000);
            m_requester->connect(serverAddress);
        }

        std::string sendReceive(const std::string& value)
        {
            m_requester->write(data_t{value});
            data_t answer = m_requester->read();
            notifyShutdown();
            return answer.at(0);
        }

        std::string breakAfterSendRequest(const std::string& value)
        {
            m_requester->write(data_t{value});
            requestKill();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return "";
        }


    };

}

#endif /* !ARTISANBUILD */


