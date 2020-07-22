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


TEST_F(TestMonitorDir, WhenNoFileISProvidedNextShouldReturnNullOpt){
    MonitorDir monitorDir(m_tempDir.dirPath(), ""); 
    EXPECT_FALSE(monitorDir.next(std::chrono::milliseconds(1)).has_value()); 
}


TEST_F(TestMonitorDir, IfOtherThreadStopsTheMonitorNextShouldRaiseException){
    MonitorDir monitorDir(m_tempDir.dirPath(), ""); 
    Tests::TestExecutionSynchronizer synchronizer(1);
    auto stopInAnotherThread = std::async(std::launch::async, [&synchronizer,&monitorDir](){
        synchronizer.notify();        
        monitorDir.stop(); 
    });
    synchronizer.waitfor(); 
    EXPECT_THROW(monitorDir.next(std::chrono::milliseconds(1000)), CommsComponent::MonitorDirClosedException); 
    stopInAnotherThread.get();
}

TEST_F(TestMonitorDir, ItShouldDetectFilesBeingMoved){
    MonitorDir monitorDir(m_tempDir.dirPath(), ""); 
    Tests::TestExecutionSynchronizer synchronizer(1);
    auto stopInAnotherThread = std::async(std::launch::async, [&synchronizer,&monitorDir, this](){
        synchronizer.notify();
        this->m_tempDir.createFileAtomically("test1.json","test1"); 
        this->m_tempDir.createFileAtomically("test2.json","test2"); 
    });
    synchronizer.waitfor(); 
    EXPECT_EQ(monitorDir.next(std::chrono::milliseconds(1000)).value(), m_tempDir.absPath("test1.json")); 
    EXPECT_EQ(monitorDir.next(std::chrono::milliseconds(1000)).value(), m_tempDir.absPath("test2.json")); 
    stopInAnotherThread.get();
}






