// Copyright 2019-2022, Sophos Limited.  All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../common/LogInitializedTests.h"
#include "safestore/IQuarantineManager.h"
#include "safestore/QuarantineManagerImpl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

//using namespace safestore;
using namespace testing;

class QuarantineManagerTests : public LogInitializedTests
{
};

TEST_F(QuarantineManagerTests, test1)
{
//    // TODO trying to get this working...
//    auto safeStoreWrapper = std::make_shared<StrictMock<MockISafeStoreWrapper>>();
//    EXPECT_CALL(*safeStoreWrapper, initialise("a","b","c")).Times(1);
//    std::shared_ptr<safestore::IQuarantineManager> quarantineManager = std::make_shared<safestore::QuarantineManagerImpl>(safeStoreWrapper);
//    quarantineManager->initialise();

}
