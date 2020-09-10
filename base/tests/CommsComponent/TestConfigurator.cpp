/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/CommsComponent/Configurator.h>
#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/TestMacros.h>

#include <sys/mount.h>


using namespace CommsComponent;

class TestConfigurator : public LogOffInitializedTests
{
    std::vector<std::string> m_mountedPaths;
public:

    void TearDown() override
    {
        for(auto& path : m_mountedPaths)
        {
            std::cout << path << "TearDown" << std::endl;
            umount2(path.c_str(), MNT_FORCE);
        }
    }

    void mountForTest(UserConf child, const std::string& chrootDir, std::vector<CommsComponent::ReadOnlyMount> listOfDependency
    , std::ostream& output)
    {
        if (CommsConfigurator::mountDependenciesReadOnly(child, listOfDependency, chrootDir, output)
            == CommsConfigurator::MountOperation::MountFailed)
        {
           std::exit(10);
        }
        for( auto dep : listOfDependency)
        {
            auto path = Common::FileSystem::join(chrootDir, dep.second);
            m_mountedPaths.push_back(path);
            std::cout << m_mountedPaths[0] << std::endl;
        }
    }
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


TEST_F(TestConfigurator, applyChildSecurityPolicyPurgesAllFilesUnderChroot) // NOLINT
{
    MAYSKIP;
    //
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_EXIT(
            {
                std::stringstream output;
                Tests::TempDir tempDir("/tmp");
                UserConf child;
                child.userName = "lp";
                child.userGroup = "lp";
                std::string chrootDir = Common::FileSystem::join(tempDir.dirPath(), "var");

                auto dirToCleanUp = Common::FileSystem::join("var", "dirToCleanUp");
                tempDir.makeDirs(dirToCleanUp);

                auto commsConfig = CommsConfigurator(chrootDir, child, child, {});
                commsConfig.applyChildSecurityPolicy();

                //path should not exist if the chroot path was cleaned up
                if (Common::FileSystem::fileSystem()->exists(Common::FileSystem::join("/", "dirToCleanUp")))
                {
                    exit(1);
                }

                exit(0);
            },
            ::testing::ExitedWithCode(0),
            ".*");
}