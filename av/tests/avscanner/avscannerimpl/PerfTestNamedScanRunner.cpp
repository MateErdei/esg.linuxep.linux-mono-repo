// Copyright 2024 Sophos Limited. All rights reserved.


#include "avscanner/avscannerimpl/NamedScanRunner.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#include <chrono>
#include <fstream>
#include <string>

#include "tests/common/RecordingMockSocket.h"
#include "av/modules/datatypes/Print.h"

#include <capnp/message.h>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;


class PerfTestNamedScanRunner
{
public:
    void runTest();
    void singleLoop(unsigned i);
    Sophos::ssplav::NamedScan::Reader createNamedScanConfig(
            ::capnp::MallocMessageBuilder& message,
            std::vector<std::string> expectedExclusions,
            bool scanHardDisc,
            bool scanNetwork,
            bool scanOptical,
            bool scanRemovable) const;
    std::vector<std::string> m_expectedExclusions;
    bool m_scanHardDisc = true;
    bool m_scanNetwork = false;
    bool m_scanOptical = false;
    bool m_scanRemovable = false;
    std::string m_expectedScanName = "testScan";

    static std::vector<std::string> getAllOtherDirs(const std::string& includedDir)
    {
        fs::path currentDir = includedDir;
        std::vector<std::string> allOtherDirs;
        do
        {
            currentDir = currentDir.parent_path();
            for (const auto& p : fs::directory_iterator(currentDir))
            {

                if (includedDir.rfind(p.path(), 0) != 0)
                {
                    if (p.status().type() == fs::file_type::directory)
                    {
                        allOtherDirs.push_back(p.path().string() + "/");
                    }
                    else
                    {
                        allOtherDirs.push_back(p.path().string());
                    }
                }
            }
        } while (currentDir != "/");
        return allOtherDirs;
    }

};

Sophos::ssplav::NamedScan::Reader PerfTestNamedScanRunner::createNamedScanConfig(
        ::capnp::MallocMessageBuilder& message,
        std::vector<std::string> expectedExclusions,
        bool scanHardDisc,
        bool scanNetwork,
        bool scanOptical,
        bool scanRemovable) const
{
    Sophos::ssplav::NamedScan::Builder scanConfigIn = message.initRoot<Sophos::ssplav::NamedScan>();
    scanConfigIn.setName(m_expectedScanName);
    auto exclusions = scanConfigIn.initExcludePaths(expectedExclusions.size());
    for (unsigned i=0; i < expectedExclusions.size(); i++)
    {
        exclusions.set(i, expectedExclusions[i]);
    }
    scanConfigIn.setScanHardDrives(scanHardDisc);
    scanConfigIn.setScanNetworkDrives(scanNetwork);
    scanConfigIn.setScanCDDVDDrives(scanOptical);
    scanConfigIn.setScanRemovableDrives(scanRemovable);

    Sophos::ssplav::NamedScan::Reader scanConfigOut = message.getRoot<Sophos::ssplav::NamedScan>();

    return scanConfigOut;
}

void PerfTestNamedScanRunner::runTest()
{
    fs::path m_testDir = "/tmp/perf/PerfTestNamedScanRunner";
    fs::create_directories(m_testDir);

    for(const auto& p: getAllOtherDirs(m_testDir))
    {
        m_expectedExclusions.push_back(p);
    }

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );

    fs::path testfile = m_testDir / "file1.txt";
    std::ofstream testfileStream(testfile.string());
    testfileStream << "this file will be included, then excluded by stem";

    std::string stemExclusion = m_testDir.parent_path().string() + "/";
    m_expectedScanName = "TestExcludeByStemScan1";

//    std::this_thread::sleep_for(std::chrono::seconds{1});
    auto start = std::chrono::high_resolution_clock::now();
    for (unsigned i=0; i<10; i++)
    {
        singleLoop(i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - start;
    PRINT("Duration ms: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());

    fs::remove_all(m_testDir);
}

void PerfTestNamedScanRunner::singleLoop(unsigned int i)
{
    PRINT("Starting " << i);
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::NamedScan::Reader scanConfigOut = createNamedScanConfig(
            message,
            m_expectedExclusions,
            m_scanHardDisc,
            m_scanNetwork,
            m_scanOptical,
            m_scanRemovable);

    NamedScanRunner runner(scanConfigOut);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();
}

int main()
{
    PerfTestNamedScanRunner tester;
    tester.runTest();
    return 0;
}
