/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/UniformIntDistribution.h>

#include "gtest/gtest.h"

using namespace Common::UtilityImpl;

TEST(UniformIntDistribution, next) // NOLINT
{
    int accumulated = 0;
    UniformIntDistribution distribution(0, 5);
    for (int i = 0; i < 1000; i++)
    {
        int curr = distribution.next();
        EXPECT_LE(curr, 5);
        EXPECT_GE(curr, 0);
        accumulated += curr;
    }

    int expected_value_of_uniform_distribution = (0 + 5) * 1000 / 2;
    EXPECT_LE(accumulated, expected_value_of_uniform_distribution + 20);
    EXPECT_GE(accumulated, expected_value_of_uniform_distribution - 20);

}
