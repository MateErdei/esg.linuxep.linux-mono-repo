/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "CommsComponent/CommsDistributor.h"
#include "CommsComponent/IOtherSideApi.h"

using namespace ::testing;

class MockCommsDistributor : public CommsComponent::CommsDistributor
{
public:
    MockCommsDistributor(const std::string& path, const std::string& filter, const std::string& responseDirPath, CommsComponent::MessageChannel& channel, CommsComponent::IOtherSideApi& childProxy) :
            CommsDistributor(path, filter, responseDirPath,channel, childProxy){};
};

