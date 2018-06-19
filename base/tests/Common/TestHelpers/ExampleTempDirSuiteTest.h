///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_EXAMPLETEMPDIRSUITETEST_H
#define EVEREST_BASE_EXAMPLETEMPDIRSUITETEST_H
#include "gtest/gtest.h"
#include "TempDir.h"
namespace Tests
{

/**
 * Helper class to allow creation of Tests with its own directory temporary directory
 * to create files and/or remove them.
 *
 * ExampleTempDirSuiteTest will create the fresh directory once and it will be shared among all the tests.
 *
 * There is no clear advantage of inheriting from this class. It is here mainly for example on how developers may achieve
 * the shared directory.
 *
 * Applications where shared directory may be interesting would be for example to create tests that only read file locations
 * Hence, it must be created once and it is safe to use later.
 *
 * Be aware that tests do not run in a fixed order, hence, it is a bad design decision to assume tests run in an specific order
 * or to have dependency across different tests.
 *
 * There is no great benefit of making this shared. Usually developers will want to define their own SetUpTestCase to create
 * all the infrastructure that must be shared among all the tests. Hence, this is just a boilerplate code on how to use.
 *
 */
    class ExampleTempDirSuiteTest : public ::testing::Test
    {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();

        static std::unique_ptr<TempDir> tempDir;
    };

}
#endif //EVEREST_BASE_EXAMPLETEMPDIRSUITETEST_H
