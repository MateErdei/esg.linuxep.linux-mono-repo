// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include <gtest/gtest.h>
#include "tests/Common/Helpers/TestExecutionSynchronizer.h"

#include <atomic>
#include <future>

namespace
{
    using namespace Tests;

    // cppcheck-suppress syntaxError
    TEST(TestExecutionSynchronizer, synchronizes2threads) // NOLINT
    {
        ReentrantExecutionSynchronizer barrier;
        std::atomic<int> value{ 0 };
        auto t1 = std::async(std::launch::async, [&barrier, &value]() {
            value.store(2, std::memory_order::memory_order_relaxed);
            value.store(3, std::memory_order::memory_order_relaxed);
            value = 2;
            value = 3;
            barrier.notify();

            barrier.waitfor(2, 10000);
            int getvalue = value.load(std::memory_order_relaxed);
            EXPECT_EQ(getvalue, 4);
        });
        auto t2 = std::async(std::launch::async, [&barrier, &value]() {
            barrier.waitfor(1, 10000);
            int getvalue = value.load(std::memory_order::memory_order_relaxed);
            EXPECT_EQ(getvalue, 3);
            value.store(4, std::memory_order::memory_order_relaxed);
            barrier.notify();
        });

        t1.get();
        t2.get();
    }

} // namespace