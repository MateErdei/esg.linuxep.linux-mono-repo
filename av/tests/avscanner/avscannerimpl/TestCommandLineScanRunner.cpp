/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/CommandLineScanRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <fstream>

namespace fs = sophos_filesystem;

namespace
{

    class ScanPathAccessor : public scan_messages::ClientScanRequest
    {
    public:
        ScanPathAccessor(const ClientScanRequest& other) // NOLINT
                : scan_messages::ClientScanRequest(other)
        {}

        operator std::string() const // NOLINT
        {
            return m_path;
        }
    };

    class RecordingMockSocket : public unixsocket::IScanningClientSocket
    {
    public:
        scan_messages::ScanResponse
        scan(datatypes::AutoFd&, const scan_messages::ClientScanRequest& request) override
        {
            std::string p = ScanPathAccessor(request);
            m_paths.emplace_back(p);
//            PRINT("Scanning " << p);
            scan_messages::ScanResponse response;
            response.setClean(true);
            return response;
        }
        std::vector<std::string> m_paths;
    };
}

TEST(CommandLineScanRunner, construction) // NOLINT
{
    std::vector<std::string> paths;
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths);
}

TEST(CommandLineScanRunner, scanRelativeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("sandbox");
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths);

    std::shared_ptr<RecordingMockSocket> socket;
    socket.reset(new RecordingMockSocket());
    runner.setSocket(socket);

    runner.run();

    fs::remove_all("sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), "sandbox/a/b/file1.txt");
}