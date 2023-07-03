// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "modules/Common/ZMQWrapperApi/IContext.h"
#include "modules/Common/ZeroMQWrapper/ISocketPublisher.h"
#include "modules/Common/ZeroMQWrapper/ISocketPublisherPtr.h"
#include "modules/Common/ZeroMQWrapper/ISocketReplier.h"
#include "modules/Common/ZeroMQWrapper/ISocketRequester.h"
#include "modules/Common/ZeroMQWrapper/ISocketRequesterPtr.h"
#include "modules/Common/ZeroMQWrapper/ISocketSubscriber.h"
#include "modules/Common/ZeroMQWrapper/ISocketSubscriberPtr.h"

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
    MOCK_METHOD(ZeroMQWrapper::ISocketSubscriberPtr, getSubscriber, (), (override));
    MOCK_METHOD(ZeroMQWrapper::ISocketPublisherPtr, getPublisher, (), (override));
    MOCK_METHOD(ZeroMQWrapper::ISocketRequesterPtr, getRequester, (), (override));
    MOCK_METHOD(ZeroMQWrapper::ISocketReplierPtr, getReplier, (), (override));
    MOCK_METHOD(ZeroMQWrapper::IProxyPtr, getProxy, (const std::string&, const std::string&), (override));
};
