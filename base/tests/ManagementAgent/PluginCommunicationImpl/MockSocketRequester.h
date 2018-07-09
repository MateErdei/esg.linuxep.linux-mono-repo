//
// Created by pair on 06/07/18.
//

#ifndef EVEREST_BASE_MOCKSOCKETREQUESTER_H
#define EVEREST_BASE_MOCKSOCKETREQUESTER_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "IPluginCallback.h"
#include "ISensorDataCallback.h"
#include <memory>
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/PluginApi/IPluginApi.h"

using namespace ::testing;

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

#endif //EVEREST_BASE_MOCKSOCKETREQUESTER_H
