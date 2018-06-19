///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "ExampleTempDirSuiteTest.h"
namespace Tests
{

    std::unique_ptr<TempDir> ExampleTempDirSuiteTest::tempDir;

    void ExampleTempDirSuiteTest::SetUpTestCase()
    {
        tempDir = TempDir::makeTempDir();
    }

    void ExampleTempDirSuiteTest::TearDownTestCase()
    {
        tempDir.reset(nullptr);
    }
}