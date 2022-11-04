SophosAddTest(TestScheduledScan
        ../../common/LogInitializedTests.cpp
        TestDaySet.cpp
        TestScanRunner.cpp
        TestScanScheduler.cpp
        TestScanSerialiser.cpp
        TestScheduleCalculations.cpp
        TestScheduledScan.cpp
        TestScheduledScanConfiguration.cpp
        TestTimeSet.cpp
        PROJECTS scanscheduler
        LIBS ${pluginapilib} common
        INC_DIRS ${pluginapiinclude}
        )
