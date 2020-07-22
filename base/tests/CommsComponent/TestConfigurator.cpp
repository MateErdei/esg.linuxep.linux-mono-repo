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
                   auto expectedMountedPath = tempDir.absPath("usr/lib/x86_64-linux-gnu/");
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
