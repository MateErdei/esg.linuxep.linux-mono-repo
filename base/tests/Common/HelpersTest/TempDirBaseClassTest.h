/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "gtest/gtest.h"

#include <tests/Common/Helpers/TempDir.h>

namespace Tests
{
    /**
     * Helper class to allow creation of Tests with its own directory temporary directory
     * to create files and/or remove them.
     * TempDirBaseClassTest will create a fresh directory for each test (TEST_F) defined.
     *
     * This test can be used by creating a new class inheriting from TempDirBaseClassTest.
     * See the tests for example.
     */
    class TempDirBaseClassTest : public ::testing::Test
    {
    public:
        /**
         * Ensure the temporary directory is created.
         */
        void SetUp() override;
        /**
         * Remove the temporary directory.
         */
        void TearDown() override;

    protected:
        /// made protected to allow classes inheriting from TempDirBaseClassTest to manipulate it.
        std::unique_ptr<TempDir> m_tempDir;
    };

} // namespace Tests
