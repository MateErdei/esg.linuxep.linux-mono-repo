// Copyright 2021-2023 Sophos Limited. All rights reserved.

// Added to ensure that the MockApiBaseServices mock class will be built
// and the build will fail if IBaseServiceApi gets updated.

#include <gtest/gtest.h>
#include "tests/Common/Helpers/MockApiBaseServices.h"

TEST(MockApiBaseServicesTtests, MockApiBaseServicesCanBeConstructed)
{
    MockApiBaseServices baseServicesMock;
}