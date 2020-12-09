/******************************************************************************************************

Copyright 2018-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Process/IProcess.h>
#include <Common/Process/IProcessException.h>
#include <Common/ProcessImpl/ProcessInfo.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>

namespace Common::Process
{
    class ProcessInfoTests: public LogOffInitializedTests{};

    // cppcheck-suppress syntaxError
    TEST_F(ProcessInfoTests, processInfoSetExecutableUserAndGroupWithValidUserAndGroupStoresCorrectResults) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;
        processInfo.setExecutableUserAndGroup("root:root");

        std::pair<bool, uid_t> userActual = processInfo.getExecutableUser();
        std::pair<bool, uid_t> groupActual = processInfo.getExecutableGroup();

        EXPECT_EQ(userActual.first, true);
        EXPECT_EQ(userActual.second, 0);

        EXPECT_EQ(groupActual.first, true);
        EXPECT_EQ(groupActual.second, 0);
    }

    TEST_F(ProcessInfoTests, processInfoSetExecutableUserAndGroupWithValidUserStoresCorrectResults) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;
        processInfo.setExecutableUserAndGroup("root");

        std::pair<bool, uid_t> userActual = processInfo.getExecutableUser();
        std::pair<bool, uid_t> groupActual = processInfo.getExecutableGroup();

        EXPECT_EQ(userActual.first, true);
        EXPECT_EQ(userActual.second, 0);

        EXPECT_EQ(groupActual.first, true);
        EXPECT_EQ(groupActual.second, 0);
    }

    TEST_F(ProcessInfoTests, processInfoSetExecutableUserAndGroupWithInvalidUserStoresCorrectResults) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;
        processInfo.setExecutableUserAndGroup("baduser");

        std::pair<bool, uid_t> userActual = processInfo.getExecutableUser();
        std::pair<bool, uid_t> groupActual = processInfo.getExecutableGroup();

        uid_t invalidUser = static_cast<uid_t>(-1);
        gid_t invalidGroup = static_cast<gid_t>(-1);

        EXPECT_EQ(userActual.first, false);
        EXPECT_EQ(userActual.second, invalidUser);

        EXPECT_EQ(groupActual.first, false);
        EXPECT_EQ(groupActual.second, invalidGroup);
    }

    TEST_F(ProcessInfoTests,
         processInfoSetExecutableUserAndGroupWithInvalidUserAndGroupStoresCorrectResults) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;
        processInfo.setExecutableUserAndGroup("baduser:badgroup");

        std::pair<bool, uid_t> userActual = processInfo.getExecutableUser();
        std::pair<bool, uid_t> groupActual = processInfo.getExecutableGroup();

        uid_t invalidUser = static_cast<uid_t>(-1);
        gid_t invalidGroup = static_cast<gid_t>(-1);

        EXPECT_EQ(userActual.first, false);
        EXPECT_EQ(userActual.second, invalidUser);

        EXPECT_EQ(groupActual.first, false);
        EXPECT_EQ(groupActual.second, invalidGroup);
    }

    TEST_F(
        ProcessInfoTests,
        processInfoSetExecutableUserAndGroupWithInvalidUserAndValidGroupStoresCorrectResults) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;
        processInfo.setExecutableUserAndGroup("baduser:root");

        std::pair<bool, uid_t> userActual = processInfo.getExecutableUser();
        std::pair<bool, uid_t> groupActual = processInfo.getExecutableGroup();

        uid_t invalidUser = static_cast<uid_t>(-1);
        gid_t invalidGroup = static_cast<gid_t>(-1);

        EXPECT_EQ(userActual.first, false);
        EXPECT_EQ(userActual.second, invalidUser);

        EXPECT_EQ(groupActual.first, false);
        EXPECT_EQ(groupActual.second, invalidGroup);
    }

    TEST_F(
        ProcessInfoTests,
        processInfoSetExecutableUserAndGroupWithValidUserAndInvalidGroupStoresCorrectResults) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;
        std::string userGroup("root:badgroup");
        processInfo.setExecutableUserAndGroup(userGroup);

        std::pair<bool, uid_t> userActual = processInfo.getExecutableUser();
        std::pair<bool, uid_t> groupActual = processInfo.getExecutableGroup();

        gid_t invalidGroup = static_cast<gid_t>(-1);

        EXPECT_EQ(userActual.first, true);
        EXPECT_EQ(userActual.second, 0);

        EXPECT_EQ(groupActual.first, false);
        EXPECT_EQ(groupActual.second, invalidGroup);

        EXPECT_EQ(processInfo.getExecutableUserAndGroupAsString(), userGroup);
    }

    TEST_F(ProcessInfoTests, setExecutableArguments) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;

        std::vector<std::string> args{ "Arg1", "Arg2", "Arg3" };

        processInfo.setExecutableArguments(args);

        ASSERT_EQ(processInfo.getExecutableArguments().size(), 3);
        EXPECT_EQ(processInfo.getExecutableArguments()[0], args[0]);
        EXPECT_EQ(processInfo.getExecutableArguments()[1], args[1]);
        EXPECT_EQ(processInfo.getExecutableArguments()[2], args[2]);
    }

    TEST_F(ProcessInfoTests, setExecutableEnvironmentVariables) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;

        Process::EnvPairs envPairs{ { "env1", "envVal1" }, { "env2", "envVal2" } };

        processInfo.setExecutableEnvironmentVariables(envPairs);

        ASSERT_EQ(processInfo.getExecutableEnvironmentVariables().size(), 2);
        EXPECT_EQ(processInfo.getExecutableEnvironmentVariables()[0].first, envPairs[0].first);
        EXPECT_EQ(processInfo.getExecutableEnvironmentVariables()[0].second, envPairs[0].second);
        EXPECT_EQ(processInfo.getExecutableEnvironmentVariables()[1].first, envPairs[1].first);
        EXPECT_EQ(processInfo.getExecutableEnvironmentVariables()[1].second, envPairs[1].second);
    }

    TEST_F(ProcessInfoTests, addExecutableArgumentsAddsNewValueCorrectly) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;

        std::string arg = "Arg1";

        processInfo.addExecutableArguments(arg);

        ASSERT_EQ(processInfo.getExecutableArguments().size(), 1);
        EXPECT_EQ(processInfo.getExecutableArguments()[0], arg);
    }

    TEST_F(ProcessInfoTests, addExecutableEnvironmentVariablesAddsNewValueCorrectly) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;

        std::string envName = "envName1";
        std::string envValue = "envValue1";

        processInfo.addExecutableEnvironmentVariables(envName, envValue);

        ASSERT_EQ(processInfo.getExecutableEnvironmentVariables().size(), 1);
        EXPECT_EQ(processInfo.getExecutableEnvironmentVariables()[0].first, envName);
        EXPECT_EQ(processInfo.getExecutableEnvironmentVariables()[0].second, envValue);
    }

    TEST_F(ProcessInfoTests, setExecutableFullPathAddsNewValueCorrectly) // NOLINT
    {
        Common::ProcessImpl::ProcessInfo processInfo;

        std::string path = "/tmp/some-path/some-exe";

        processInfo.setExecutableFullPath(path);

        EXPECT_EQ(processInfo.getExecutableFullPath(), path);
    }
} // namespace Common::Process