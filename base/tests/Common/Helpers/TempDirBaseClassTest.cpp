/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "tests/Common/Helpers/TempDirBaseClassTest.h"

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
} // namespace Tests