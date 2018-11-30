/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include <Common/PluginApi/IRawDataCallback.h>
#include <Common/PluginApiImpl/AbstractDetectorDataCallback.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"


class MockDetectorDataCallback: public Common::PluginApi::IRawDataCallback, Common::PluginApiImpl::AbstractDetectorDataCallback
{
public:
    MOCK_METHOD1(receiveData, void(Common::EventTypes::IEventType* eventTypes));
    MOCK_METHOD1(processIncomingCredentialData, Common::EventTypesImpl::CredentialEvent (const std::vector<std::string>& data));
    MOCK_METHOD1(processIncomingPortScanningData, Common::EventTypesImpl::PortScanningEvent (const std::vector<std::string>& data));
};


