///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "TempDirBaseClassTest.h"
namespace Tests
{

    void TempDirBaseClassTest::SetUp()
    {
        Test::SetUp();
        m_tempDir = TempDir::makeTempDir();
    }

    void TempDirBaseClassTest::TearDown()
    {
        m_tempDir.reset(nullptr);
        Test::TearDown();
    }
}