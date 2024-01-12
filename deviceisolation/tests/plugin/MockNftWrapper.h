// Copyright 2024 Sophos All rights reserved.

#pragma once

#include "deviceisolation/modules/plugin/INftWrapper.h"
#include "deviceisolation/modules/pluginimpl/IsolationExclusion.h"
#include <gmock/gmock.h>

using namespace ::testing;

class MockNftWrapper : public Plugin::INftWrapper
{
public:
    MOCK_METHOD(Plugin::IsolateResult, applyIsolateRules, (const std::vector<Plugin::IsolationExclusion>&), (override));
    MOCK_METHOD(Plugin::IsolateResult, clearIsolateRules, (), (override));
};