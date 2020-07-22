/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/MonitorDir.h>
#include <tests/Common/Helpers/TestExecutionSynchronizer.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/TempDir.h>
using namespace CommsComponent;
class TestMonitorDir : public LogOffInitializedTests{
    public: 
    Tests::TempDir m_tempDir; 
    TestMonitorDir():m_tempDir("/tmp","TestMonitorDir"){}
};

TEST_F(TestMonitorDir, ConstructorAndDestructorShouldWorkWithoutHanging){
    {
        MonitorDir monitorDir(m_tempDir.dirPath(), ""); 
    }
}


