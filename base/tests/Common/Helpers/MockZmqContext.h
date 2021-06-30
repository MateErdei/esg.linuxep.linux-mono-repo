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

    ZeroMQWrapper::ISocketSubscriberPtr m_subscriber;
    ZeroMQWrapper::ISocketPublisherPtr m_publisher;
    ZeroMQWrapper::ISocketRequesterPtr m_requester;
    ZeroMQWrapper::ISocketReplierPtr m_replier;
    virtual ZeroMQWrapper::ISocketSubscriberPtr getSubscriber()
    {
        return std::move(m_subscriber);

    }
    virtual ZeroMQWrapper::ISocketPublisherPtr getPublisher()
    {

        return std::move(m_publisher);
    }
    virtual ZeroMQWrapper::ISocketRequesterPtr getRequester()
    {
        return std::move(m_requester);
    }
    virtual ZeroMQWrapper::ISocketReplierPtr getReplier()
    {
        return std::move(m_replier);
    }

    MOCK_METHOD2(getProxy, ZeroMQWrapper::IProxyPtr(const std::string&, const std::string&));

};
