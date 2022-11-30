// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "PluginMemoryAppenderUsingTests.h"

#include "pluginimpl/PolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

namespace fs = sophos_filesystem;

namespace
{
    class PolicyProcessorUnitTestClass : public Plugin::PolicyProcessor
    {
    public:
        PolicyProcessorUnitTestClass() : Plugin::PolicyProcessor(nullptr) {}

    protected:
        void notifyOnAccessProcessIfRequired() override { PRINT("Notified soapd"); }
    };

    class TestPolicyProcessorBase : public PluginMemoryAppenderUsingTests
    {
    protected:
        fs::path m_testDir;

    };
}
