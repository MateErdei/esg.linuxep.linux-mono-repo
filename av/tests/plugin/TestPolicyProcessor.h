// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include "PluginMemoryAppenderUsingTests.h"

#include "pluginimpl/PolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/MockFileSystem.h"

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
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
        std::string m_customerIdPath;

        void createTestDir()
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
            appConfig.setData("PLUGIN_INSTALL", m_testDir );
        }

        void setupBase()
        {
            createTestDir();
            m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
            m_customerIdPath = m_testDir / "var/customer_id.txt";
        }
    };
}
