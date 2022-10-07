// Copyright 2019-2022, Sophos Limited.  All rights reserved.

#include "MockISafeStoreWrapper"

#include "../common/LogInitializedTests.h"
#include "safestore/IQuarantineManager.h"
#include "safestore/QuarantineManagerImpl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace safestore;
using namespace testing;

class QuarantineManagerTests : public LogInitializedTests
{
};

TEST_F(QuarantineManagerTests, test1)
{

    std::shared_ptr<ISafeStoreWrapper> safeStoreWrapper = std::shared_ptr<StrictMock<MockISafeStoreWrapper>>();
    std::shared_ptr<IQuarantineManager> quarantineManager = std::make_shared<QuarantineManagerImpl>(safeStoreWrapper);
//    quarantineManager->initialise();

}
