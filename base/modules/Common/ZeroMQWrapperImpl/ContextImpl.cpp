//
// Created by pair on 06/06/18.
//

#include "ContextImpl.h"

#include <Common/ZeroMQ_wrapper/ISocketSubscriber.h>
#include <Common/ZeroMQ_wrapper/ISocketPublisher.h>
#include <Common/ZeroMQ_wrapper/ISocketRequester.h>
#include <Common/ZeroMQ_wrapper/ISocketReplier.h>

#include <zmq.h>

using namespace Common::ZeroMQWrapperImpl;

std::unique_ptr<Common::ZeroMQ_wrapper::ISocketSubscriber> ContextImpl::getSubscriber(const std::string &address)
{
    return std::unique_ptr<ZeroMQ_wrapper::ISocketSubscriber>();
}

std::unique_ptr<Common::ZeroMQ_wrapper::ISocketPublisher> ContextImpl::getPublisher(const std::string &address)
{
    return std::unique_ptr<ZeroMQ_wrapper::ISocketPublisher>();
}

std::unique_ptr<Common::ZeroMQ_wrapper::ISocketRequester> ContextImpl::getRequester(const std::string &address)
{
    return std::unique_ptr<ZeroMQ_wrapper::ISocketRequester>();
}

std::unique_ptr<Common::ZeroMQ_wrapper::ISocketReplier> ContextImpl::getReplier(const std::string &address)
{
    return std::unique_ptr<ZeroMQ_wrapper::ISocketReplier>();
}
