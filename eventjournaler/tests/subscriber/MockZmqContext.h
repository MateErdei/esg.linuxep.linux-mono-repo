/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#pragma once

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketSubscriber.h"
#include "Common/ZeroMQWrapper/ISocketPublisher.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace Common;


class MockSocketRequester : public Common::ZeroMQWrapper::ISocketRequester
{
public:
    MOCK_METHOD0(read, std::vector<std::string>());
    MOCK_METHOD1(write, void(const std::vector<std::string>&));
    MOCK_METHOD0(fd, int());
    MOCK_METHOD1(setTimeout, void(int));
    MOCK_METHOD1(setConnectionTimeout, void(int));
    MOCK_METHOD1(connect, void(const std::string&));
    MOCK_METHOD1(listen, void(const std::string&));
};

class MockSocketSubscriber : public Common::ZeroMQWrapper::ISocketSubscriber
{
public:
    MOCK_METHOD1(subscribeTo, void(const std::string& subject));
    MOCK_METHOD0(read, std::vector<std::string>());
    MOCK_METHOD1(write, void(const std::vector<std::string>&));
    MOCK_METHOD0(fd, int());
    MOCK_METHOD1(setTimeout, void(int));
    MOCK_METHOD1(setConnectionTimeout, void(int));
    MOCK_METHOD1(connect, void(const std::string&));
    MOCK_METHOD1(listen, void(const std::string&));
};

class MockSocketPublisher : public Common::ZeroMQWrapper::ISocketPublisher
{
public:
    MOCK_METHOD1(subscribeTo, void(const std::string& subject));
    MOCK_METHOD0(read, std::vector<std::string>());
    MOCK_METHOD1(write, void(const std::vector<std::string>&));
    MOCK_METHOD0(fd, int());
    MOCK_METHOD1(setTimeout, void(int));
    MOCK_METHOD1(setConnectionTimeout, void(int));
    MOCK_METHOD1(connect, void(const std::string&));
    MOCK_METHOD1(listen, void(const std::string&));
};

class MockSocketReplier : public Common::ZeroMQWrapper::ISocketReplier
{
public:
    MOCK_METHOD1(subscribeTo, void(const std::string& subject));
    MOCK_METHOD0(read, std::vector<std::string>());
    MOCK_METHOD1(write, void(const std::vector<std::string>&));
    MOCK_METHOD0(fd, int());
    MOCK_METHOD1(setTimeout, void(int));
    MOCK_METHOD1(setConnectionTimeout, void(int));
    MOCK_METHOD1(connect, void(const std::string&));
    MOCK_METHOD1(listen, void(const std::string&));
};


class MockZmqContext : public ZMQWrapperApi::IContext
{
public:

//    MockSocketSubscriber*  subscriber = new StrictMock<MockSocketSubscriber>();
    //ZeroMQWrapper::ISocketSubscriberPtr a = std::make_unique<subscriber>();

//    MockZmqContext(ZeroMQWrapper::ISocketSubscriberPtr subscriber,
//                   ZeroMQWrapper::ISocketPublisherPtr publisher,
//                   ZeroMQWrapper::ISocketRequesterPtr requester,
//                   ZeroMQWrapper::ISocketReplierPtr replier
//                   ): m_subscriber(std::move(subscriber)),
//                      m_publisher(std::move(publisher)),
//                      m_requester(std::move(requester)),
//                      m_replier(std::move(replier))
//                    {}
//    ~MockZmqContext(){}
//    virtual std::unique_ptr<IMyObjectThing> nonCopyableReturn() {
//        return std::unique_ptr<IMyObjectThing>(nonCopyableReturnProxy());
//    }
//    MockSocketSubscriber*  Socketsubscriber = new StrictMock<MockSocketSubscriber>();
    ZeroMQWrapper::ISocketSubscriberPtr m_subscriber;
    ZeroMQWrapper::ISocketPublisherPtr m_publisher;
    ZeroMQWrapper::ISocketRequesterPtr m_requester;
    ZeroMQWrapper::ISocketReplierPtr m_replier;
    virtual ZeroMQWrapper::ISocketSubscriberPtr getSubscriber()
    {
        //ZeroMQWrapper::ISocketSubscriberPtr a = std::make_unique<MockSocketSubscriber>();
//        return subscriber;

        //    return Common::ZeroMQWrapper::ISocketSubscriberPtr(new Common::ZeroMQWrapperImpl::SocketSubscriberImpl(m_context));
        return std::move(m_subscriber);

//        auto a = new MockSocketRequester();
//        return a;
//        return ZeroMQWrapper::ISocketSubscriberPtr(stdLLa);
    }
    virtual ZeroMQWrapper::ISocketPublisherPtr getPublisher()
    {
        //return std::make_unique<MockSocketPublisher>();
        return std::move(m_publisher);
    }
    virtual ZeroMQWrapper::ISocketRequesterPtr getRequester()
    {
//        ZeroMQWrapper::ISocketRequesterPtr a = std::make_unique<MockSocketRequester>();
//        return a;
//        return ZeroMQWrapper::ISocketRequesterPtr(getRequesterMocked());
        return std::move(m_requester);
    }
    virtual ZeroMQWrapper::ISocketReplierPtr getReplier()
    {
        //return std::make_unique<MockSocketReplier>();
        return std::move(m_replier);
    }

//    virtual ZeroMQWrapper::IProxyPtr getProxy(const std::string& frontend, const std::string& backend)
//    {
//        return ZeroMQWrapper::IProxyPtr(getProxyMocked(frontend, backend));
//    }

//    MOCK_METHOD0(getSubscriber, ZeroMQWrapper::ISocketSubscriberPtr(void));
//    MOCK_METHOD0(getPublisher, ZeroMQWrapper::ISocketPublisherPtr(void));
//    MOCK_METHOD0(getRequester, ZeroMQWrapper::ISocketRequesterPtr(void));
//    MOCK_METHOD0(getReplier, ZeroMQWrapper::ISocketReplierPtr(void));
//    MOCK_METHOD0(createContext, ZMQWrapperApi::IContextSharedPtr(void));
    MOCK_METHOD2(getProxy, ZeroMQWrapper::IProxyPtr(const std::string&, const std::string&));

//    virtual ZeroMQWrapper::ISocketSubscriberPtr getSubscriber() = 0;
//    virtual ZeroMQWrapper::ISocketPublisherPtr getPublisher() = 0;
//    virtual ZeroMQWrapper::ISocketRequesterPtr getRequester() = 0;
//    virtual ZeroMQWrapper::ISocketReplierPtr getReplier() = 0;
//    virtual ZeroMQWrapper::IProxyPtr getProxy(const std::string& frontend, const std::string& backend) = 0;
//};
//
//extern ZMQWrapperApi::IContextSharedPtr createContext();
};
