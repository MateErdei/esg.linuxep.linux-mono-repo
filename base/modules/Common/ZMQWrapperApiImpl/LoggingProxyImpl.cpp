/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "LoggingProxyImpl.h"

#include "Logger.h"

#include "modules/Common/ZeroMQWrapper/ISocketSubscriber.h"
#include "modules/Common/ZeroMQWrapperImpl/SocketImpl.h"

#include <zmq.h>

namespace
{
    class DebugLogHandler : public Common::Reactor::ICallbackListener
    {
    public:
        void messageHandler(Common::ZeroMQWrapper::IReadable::data_t request) override
        {
            std::stringstream message;
            message << "Pub Sub Log : ";
            // Subscription and unsubscription is sent as a single message with the first char set to 1 or 0
            // respectively
            if (request.size() == 1)
            {
                std::string& data = request[0];
                if (!data.empty() && (data[0] == char(0) || data[0] == char(1)))
                {
                    message << (data[0] == 0 ? "Unsubscribe " : "Subscribe ") << data.substr(1, std::string::npos);
                }
                else // Not a subscription message
                {
                    message << data;
                }
            }
            else
            {
                for (const std::string& data : request)
                {
                    message << data << " ";
                }
            }
            LOGDEBUG(message.str());
        }
    };

    class SocketPullImpl : public Common::ZeroMQWrapperImpl::SocketImpl,
                           virtual public Common::ZeroMQWrapper::ISocketSubscriber
    {
    public:
        explicit SocketPullImpl(Common::ZeroMQWrapperImpl::ContextHolderSharedPtr context) :
            SocketImpl(std::move(context), ZMQ_PULL)
        {
        }

        /**
         * Read event from the socket
         * @return
         */
        std::vector<std::string> read() override { return Common::ZeroMQWrapperImpl::SocketUtil::read(m_socket); }

        /**
         * Set the subscription for this socket.
         * @param subject
         * Dummy as not used with pull socket!
         */
        void subscribeTo(const std::string& subject) override { LOGDEBUG(subject); }
    };

} // namespace

namespace Common
{
    namespace ZMQWrapperApiImpl
    {
        LoggingProxyImpl::LoggingProxyImpl(
            const std::string& frontend,
            const std::string& backend,
            Common::ZeroMQWrapperImpl::ContextHolderSharedPtr context) :
            ProxyImpl(frontend, backend, context),
            m_captureAddress("inproc://Capture"),
            m_capture(context, ZMQ_PUSH),
            m_captureListener(new SocketPullImpl(context)),
            m_debugLoggerCallbackPtr(new DebugLogHandler),
            m_reactor(Common::Reactor::createReactor())
        {
            Common::ZeroMQWrapperImpl::SocketUtil::listen(m_capture, m_captureAddress);
            m_captureListener->connect(m_captureAddress);
            m_reactor->addListener(m_captureListener.get(), m_debugLoggerCallbackPtr.get());
            m_captureZMQSocket = m_capture.skt();
        }

        void LoggingProxyImpl::start()
        {
            m_reactor->start();
            ProxyImpl::start();
        }

        void LoggingProxyImpl::stop()
        {
            ProxyImpl::stop();
            m_reactor->stop();
            m_reactor->join();
        }
    } // namespace ZMQWrapperApiImpl
} // namespace Common
