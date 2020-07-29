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

    void createResponseJsonFile(const std::string& jsonContent, const std::string& destination, const std::string& midPoint)
    {
        // silence unused variable warning
        (void)midPoint;
        // use the response directory as a midpoint for the atomic write so we don't have to create /opt/sophos-spl/tmp
        CommsComponent::CommsDistributor::createResponseJsonFile(jsonContent, destination, m_responseDirPath);
    }
};

