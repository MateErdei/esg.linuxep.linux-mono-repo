/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/Configurator.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestMacros.h>

#include <thread>



using namespace CommsComponent;

class TestConfigurator : public LogOffInitializedTests
{
};

TEST_F(TestConfigurator, MountDependenciesReadOnly) // NOLINT
{
   MAYSKIP;
//
   ::testing::FLAGS_gtest_death_test_style = "threadsafe";
   ASSERT_EXIT({
                   std::stringstream output;
                   Tests::TempDir tempDir("/tmp");
                   UserConf child;
                   child.userName = "lp";
                   child.userGroup = "lp";

                   auto listOfDependency = CommsConfigurator::getListOfDependenciesToMount();
                   if (CommsConfigurator::mountDependenciesReadOnly(child, listOfDependency, tempDir.dirPath(), output)
                       == CommsConfigurator::MountOperation::MountFailed)
                   {
                       exit(1);
                   }
                   auto expectedMountedPath = tempDir.absPath("usr/lib/systemd/");
                   // mounted, hence, folder should be available anymore. as the path /usr/lib has been mounted.
                   if (!Common::FileSystem::fileSystem()->exists(expectedMountedPath))
                   {
            exit(2); 
        }
        CommsConfigurator::cleanDefaultMountedPaths(tempDir.dirPath()); 

        // unmounted, hence, folder should not be available anymore. 
        if (Common::FileSystem::fileSystem()->exists(expectedMountedPath))
        {
            exit(3); 
        }

        exit(0); 
  },
               ::testing::ExitedWithCode(0), ".*");
}
//
//TEST_F(TestConfigurator, cleanDefaultMountedPaths) // NOLINT
//{
//    MAYSKIP;
//    //
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    ASSERT_EXIT(
//            {
//                std::stringstream output;
//                //Tests::TempDir tempDir("/tmp");
//                UserConf child;
//                child.userName = "lp";
//                child.userGroup = "lp";
//                std::string chrootDir = Common::FileSystem::join("/tmp/tempMKxx7l/", "var");
//
//
//                auto listOfDependency = CommsConfigurator::getListOfDependenciesToMount();
//
//                if (CommsConfigurator::mountDependenciesReadOnly(child, listOfDependency, chrootDir, output) ==
//                CommsConfigurator::MountOperation::MountFailed)
//                {
//                    exit(1);
//                }
//                auto expectedMountedPath = "/tmp/tempMKxx7l/var/opt/foldermount/foledr2/";
//                if (CommsConfigurator::mountDependenciesReadOnly(child, listOfDependency, chrootDir, output) ==
//                    CommsConfigurator::MountOperation::MountFailed)
//                {
//                    exit(1);
//                }
//                if (!Common::FileSystem::fileSystem()->exists(expectedMountedPath))
//                {
//                    exit(2);
//                }
//                auto commsConfig = CommsConfigurator(chrootDir, child, child, listOfDependency);
//                commsConfig.applyChildSecurityPolicy();
//
//                // mounted, hence, folder should be available anymore. as the path /usr/lib has been mounted.
//                if (!Common::FileSystem::fileSystem()->exists(expectedMountedPath))
//                {
//                    exit(3);
//                }
//                std::cout << chrootDir << std::endl;
//                std::this_thread::sleep_for(std::chrono::seconds(3));
//                CommsConfigurator::cleanDefaultMountedPaths(chrootDir);
//
//                // unmounted, hence, folder should not be available anymore.
//                std::cout << "unmounted" << std::endl;
//                if (Common::FileSystem::fileSystem()->exists(expectedMountedPath))
//                {
//                    std::cout << "unmount failed" << std::endl;
//                    std::this_thread::sleep_for(std::chrono::seconds(3));
//                    exit(4);
//                }
//
//                exit(0);
//            },
//            ::testing::ExitedWithCode(0),
//            ".*");
//}
//TEST_F(TestConfigurator, applyChildSecurityPolicy) // NOLINT
//{
//    MAYSKIP;
//    //
//    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
//    ASSERT_EXIT(
//            {
//                std::stringstream output;
//                //Tests::TempDir tempDir("/tmp");
//                UserConf child;
//                child.userName = "lp";
//                child.userGroup = "lp";
//                child.logName = "child";
//                std::string chrootDir = Common::FileSystem::join("/tmp/tempMKxx7l/", "var");
//                setenv("SOPHOS_INSTALL", "/tmp/tempMKxx7l", 1);
//
//                char buf [PATH_MAX];
//                std::string cwd = getcwd(buf, PATH_MAX);
//                std::cout << cwd << std::endl;
//
//                std::vector<CommsComponent::ReadOnlyMount> listOfDependency1;
//                //listOfDependency1.push_back({"/opt/test/test.txt", "opt/test/test.txt"});
//                listOfDependency1.push_back({"/opt/foldermount/", "test/opt/foldermount/"});
//                if (CommsConfigurator::mountDependenciesReadOnly(child, listOfDependency1, chrootDir, output) ==
//                    CommsConfigurator::MountOperation::MountFailed)
//                {
//                    exit(1);
//                }
//
//                auto expectedMountedPath = Common::FileSystem::join(chrootDir, "test/opt/foldermount/foledr2/");
//                if (!Common::FileSystem::fileSystem()->exists(expectedMountedPath))
//                {
//                    exit(2);
//                }
//
//                std::vector<CommsComponent::ReadOnlyMount> listOfDependency2;
//                listOfDependency2.push_back({"/opt/foldermount/", "test/opt/foldermount/"});
//                auto commsConfig = CommsConfigurator(chrootDir, child, child, listOfDependency2);
//                commsConfig.applyChildSecurityPolicy();
//
//                std::string cwd2 = getcwd(buf, PATH_MAX);
//                std::cout << "current working directory  " << cwd2 << std::endl;
//                // mounted, hence, folder should be available anymore. as the path /usr/lib has been mounted.
//                if (!Common::FileSystem::fileSystem()->exists("/test/opt/foldermount/foledr2/"))
//                {
//                    exit(3);
//                }
//                std::cout << chrootDir << std::endl;
//                chroot(cwd.c_str());
//                chdir(cwd.c_str());
//                setenv("SOPHOS_INSTALL", "/tmp/tempMKxx7l", 1);
//                CommsConfigurator::cleanDefaultMountedPaths(chrootDir);
//
//                // unmounted, hence, folder should not be available anymore.
//                std::cout << "unmounted" << std::endl;
//                if (Common::FileSystem::fileSystem()->exists("/test/opt/foldermount/foledr2/"))
//                {
//                    std::cout << "unmount failed" << std::endl;
//                    std::cout << "working directory  " << getcwd(buf, PATH_MAX) << std::endl;
//                    std::this_thread::sleep_for(std::chrono::seconds(3));
//                    exit(4);
//                }
//
//                exit(0);
//            },
//            ::testing::ExitedWithCode(0),
//            ".*");
//}