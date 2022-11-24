// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "MockSusiApi.h"

#include "../../common/LogInitializedTests.h"
#include "common/MemoryAppender.h"
#include "sophos_threat_detector/threat_scanner/SusiGlobalHandler.h"

#include <gtest/gtest.h>

using namespace  threat_scanner;

namespace
{
    class TestSusiGlobalHandler : public MemoryAppenderUsingTests
    {
    public:
        TestSusiGlobalHandler() : MemoryAppenderUsingTests("ThreatScanner") {}
    };
}

TEST_F(TestSusiGlobalHandler, testConstructor)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    ASSERT_NO_THROW(auto temp = SusiGlobalHandler(mockSusiApi));
}

TEST_F(TestSusiGlobalHandler, testBootstrapping)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);
    ASSERT_EQ(SUSI_S_OK, globalHandler.bootstrap());
}

TEST_F(TestSusiGlobalHandler, testBootstrappingFails)
{
    auto mockSusiApi = std::make_shared<StrictMock<MockSusiApi>>();

    EXPECT_CALL(*mockSusiApi, SUSI_SetLogCallback(_)).WillRepeatedly(Return(SUSI_S_OK));

    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_CALL(*mockSusiApi, SUSI_Install(_,_)).WillOnce(Return(SUSI_E_INTERNAL));

    ASSERT_EQ(SUSI_E_INTERNAL, globalHandler.bootstrap());
}

TEST_F(TestSusiGlobalHandler, testInitializeSusi)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    std::string resultJson;
    SusiVersionResult result = { .version = 1, .versionResultJson = resultJson.data() };

    EXPECT_CALL(*mockSusiApi, SUSI_GetVersion(_)).WillOnce(::testing::DoAll(testing::SetArgPointee<0>(&result), Return(SUSI_S_OK)));
    ASSERT_EQ(true, globalHandler.initializeSusi(""));
}

TEST_F(TestSusiGlobalHandler, testInitializeSusiFirstBootStrapFails)
{
    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_CALL(*mockSusiApi, SUSI_Install(_,_)).WillOnce(Return(SUSI_E_INTERNAL));
    EXPECT_THROW(globalHandler.initializeSusi(""), std::runtime_error);
}

TEST_F(TestSusiGlobalHandler, testInitializeSusiFirstInitFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    std::string resultJson;
    SusiVersionResult result = { .version = 1, .versionResultJson = resultJson.data() };

    EXPECT_CALL(*mockSusiApi, SUSI_Initialize(_,_))
        .WillOnce(Return(SUSI_E_INTERNAL)).
        WillOnce(Return(SUSI_S_OK));
    EXPECT_CALL(*mockSusiApi, SUSI_GetVersion(_))
        .WillOnce(::testing::DoAll(testing::SetArgPointee<0>(&result), Return(SUSI_S_OK)));
    ASSERT_EQ(true, globalHandler.initializeSusi(""));

    EXPECT_TRUE(appenderContains("Bootstrapping SUSI from update source: \"/susi/update_source\"", 2));
    EXPECT_TRUE(appenderContains("Attempting to re-install SUSI"));
    EXPECT_TRUE(appenderContains("Initialising Global Susi successful", 1));
}

TEST_F(TestSusiGlobalHandler, testInitializeSusiAllInitFail)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto mockSusiApi = std::make_shared<NiceMock<MockSusiApi>>();
    auto globalHandler = SusiGlobalHandler(mockSusiApi);

    EXPECT_CALL(*mockSusiApi, SUSI_Initialize(_,_)).WillRepeatedly(Return(SUSI_E_INTERNAL));
    EXPECT_THROW(globalHandler.initializeSusi(""), std::runtime_error);
    EXPECT_TRUE(appenderContains("Bootstrapping SUSI from update source: \"/susi/update_source\"", 2));
    EXPECT_FALSE(appenderContains("Initialising Global Susi successful"));
}