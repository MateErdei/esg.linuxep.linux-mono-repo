/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/UniformIntDistribution.h>

#include "gtest/gtest.h"
#include <chrono>
#include <thread>

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
    EXPECT_LE(accumulated, expected_value_of_uniform_distribution + 200);
    EXPECT_GE(accumulated, expected_value_of_uniform_distribution - 200);

}

TEST(UniformIntDistribution, testConstructorGeneratesARandomSeed) // NOLINT
{
    //This for loop can be used for stress testing the UniformIntDistribution
    for( int j=0; j< 1; j++) // already tested with a loop of 10000 iterations
    {
        UniformIntDistribution distribution(0, 1000);
        const int NumDist = 10;
        // if seed was 0 then distribution next would return 0.
        if (distribution.next() == 0)
        {
            std::vector<std::unique_ptr<UniformIntDistribution>> distributions;
            for (int i = 0; i < NumDist; i++)
            {
                distributions.emplace_back(new UniformIntDistribution(0, 100000));
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            bool pass = false;
            int first = distributions[0]->next();
            for (int i = 1; i < NumDist; i++)
            {
                if (distributions[i]->next() != first)
                {
                    pass = true;
                    break;
                }
            }

            EXPECT_TRUE(pass) << "It should be highly improbably to get" << NumDist
                              << " the distributions equal. Value: " << first << " iteration: " << j;
        }
    }
}
