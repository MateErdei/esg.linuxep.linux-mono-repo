/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Executable
 */

#include "MockHttpSender.h"
#include "MockTelemetryProvider.h"

#include <Telemetry/TelemetryImpl/Telemetry.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/Telemetry/TelemetryImpl/TelemetryProcessor.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using ::testing::StrictMock;

using namespace Common::HttpSenderImpl;

class TelemetryProcessorTest : public ::testing::Test
{
public:
    const std::string m_jsonFilePath = "/opt/sophos-spl/base/telemetry/var/telemetry.json";
    MockFilePermissions* m_mockFilePermissions = nullptr;
    MockFileSystem* m_mockFileSystem = nullptr;
    const std::size_t m_maxJsonBytes = 1000;

    void SetUp() override
    {
        std::unique_ptr<MockFilePermissions> mockfilePermissions(new StrictMock<MockFilePermissions>());
        m_mockFilePermissions = mockfilePermissions.get();
        Tests::replaceFilePermissions(std::move(mockfilePermissions));
        std::unique_ptr<MockFileSystem> mockfileSystem(new StrictMock<MockFileSystem>());
        m_mockFileSystem = mockfileSystem.get();
        Tests::replaceFileSystem(std::move(mockfileSystem));
    }

    void TearDown() override { Tests::restoreFileSystem(); }
};

TEST_F(TelemetryProcessorTest, telemetryProcessorNoProviders) // NOLINT
{
    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ("{}", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorOneProvider) // NOLINT
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock":{"key":1}})", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorTwoProviders) // NOLINT
{
    auto mockTelemetryProvider1 = std::make_shared<MockTelemetryProvider>();
    auto mockTelemetryProvider2 = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider1, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider1, getName()).WillOnce(Return("Mock1"));

    EXPECT_CALL(*mockTelemetryProvider2, getTelemetry()).WillOnce(Return(R"({"key":2})"));
    EXPECT_CALL(*mockTelemetryProvider2, getName()).WillOnce(Return("Mock2"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider1);
    telemetryProviders.emplace_back(mockTelemetryProvider2);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock1":{"key":1},"Mock2":{"key":2}})", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorThreeProvidersOneThrows) // NOLINT
{
    auto mockTelemetryProvider1 = std::make_shared<MockTelemetryProvider>();
    auto mockTelemetryProvider2 = std::make_shared<MockTelemetryProvider>();
    auto mockTelemetryProvider3 = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*mockTelemetryProvider1, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider1, getName()).WillOnce(Return("Mock1"));

    EXPECT_CALL(*mockTelemetryProvider2, getTelemetry()).WillOnce(Throw(std::runtime_error("badProvider")));

    EXPECT_CALL(*mockTelemetryProvider3, getTelemetry()).WillOnce(Return(R"({"key":3})"));
    EXPECT_CALL(*mockTelemetryProvider3, getName()).WillOnce(Return("Mock3"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;

    telemetryProviders.emplace_back(mockTelemetryProvider1);
    telemetryProviders.emplace_back(mockTelemetryProvider2);
    telemetryProviders.emplace_back(mockTelemetryProvider3);

    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);
    telemetryProcessor.gatherTelemetry();
    std::string json = telemetryProcessor.getSerialisedTelemetry();
    ASSERT_EQ(R"({"Mock1":{"key":1},"Mock3":{"key":3}})", json);
}

TEST_F(TelemetryProcessorTest, telemetryProcessorWriteOutJson) // NOLINT
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    EXPECT_CALL(*m_mockFileSystem, writeFile(m_jsonFilePath, R"({"Mock":{"key":1}})")).Times(testing::AtLeast(1));
    EXPECT_CALL(*m_mockFilePermissions, chmod(m_jsonFilePath, S_IRUSR | S_IWUSR)).Times(testing::AtLeast(1));

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(R"({"key":1})"));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);
    telemetryProcessor.gatherTelemetry();
    telemetryProcessor.saveAndSendTelemetry();
}

TEST_F(TelemetryProcessorTest, telemetryProcessorDoesNotProcessLargeData) // NOLINT
{
    auto mockTelemetryProvider = std::make_shared<MockTelemetryProvider>();

    std::string longString = std::string(m_maxJsonBytes, 'a');

    std::stringstream ss;
    ss << R"({"key":")" << longString << R"("})";

    EXPECT_CALL(*mockTelemetryProvider, getTelemetry()).WillOnce(Return(ss.str()));
    EXPECT_CALL(*mockTelemetryProvider, getName()).WillOnce(Return("Mock"));

    std::vector<std::shared_ptr<Telemetry::ITelemetryProvider>> telemetryProviders;
    telemetryProviders.emplace_back(mockTelemetryProvider);
    Telemetry::TelemetryProcessor telemetryProcessor(telemetryProviders, m_maxJsonBytes);
    telemetryProcessor.gatherTelemetry();
    EXPECT_THROW(telemetryProcessor.saveAndSendTelemetry(), std::invalid_argument); // NOLINT
}
