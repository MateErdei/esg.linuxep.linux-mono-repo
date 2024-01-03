// Copyright 2023 Sophos Limited. All rights reserved.

/*
 * Send telemetry with a set of fixed fake files
 */

#include "telemetry/Telemetry.h"
#include "Common/ConfigFile/ConfigFile.h"
#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/Datatypes/Print.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#include <log4cplus/logger.h>

#include <fstream>

#include <gtest/gtest.h>

#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockPlatformUtils.h"

using namespace Common::ConfigFile;
using namespace thininstaller::telemetry;

namespace
{
    std::string expectConfig(MockFileSystem& fs)
    {
        const auto CONFIG_FILE = "/config.json";
        ConfigFile::lines_t lines;
        lines.emplace_back("TOKEN=58d3748d1cba0d41608a5474bb1f2b1170f8672cbf07504e7773f52c86a5be28");
        lines.emplace_back("URL=https://mcs2-cloudstation-us-west-2.qa.hydra.sophos.com/sophos/management/ep");
        lines.emplace_back("PRODUCTS=all");
        lines.emplace_back("SDDS3_SUS_URL=sus.sophosupd.com");
        lines.emplace_back("SDDS3_CONTENT_URLS=sdds3.sophosupd.com;sdds3.sophosupd.net");
        lines.emplace_back("CUSTOMER_ID=9fa0f108-a0c2-f4f8-dad4-647097013636");
        lines.emplace_back("TENANT_ID=abcdef00-0000-1111-2222-333333333333");
        lines.emplace_back("CUSTOMER_TOKEN=c02e7f96-26fc-4960-aadb-a3d5fe81b183");
        EXPECT_CALL(fs, readLines(CONFIG_FILE)).WillRepeatedly(Return(lines));
        return CONFIG_FILE;
    }

    std::string expectArgs(MockFileSystem& fs)
    {
        constexpr auto ARGS_FILE = "/thininstallerArgs.ini";
        ConfigFile::lines_t lines;
        lines.emplace_back("allow-override-mcs-ca = true");
        EXPECT_CALL(fs, readLines(ARGS_FILE)).WillRepeatedly(Return(lines));
        EXPECT_CALL(fs, readFile(ARGS_FILE)).WillRepeatedly(Return(ARGS_FILE));
        return ARGS_FILE;
    }

    std::string expectReport(MockFileSystem* fs)
    {
        constexpr auto REPORT_FILE = "/thininstaller_report.ini";
        ConfigFile::lines_t lines;
        lines.emplace_back("installedPackagesVerified = true");
        lines.emplace_back("glibcVersionVerified = true");
        lines.emplace_back("sufficientDiskSpace = true");
        lines.emplace_back("sufficientMemory = true");
        lines.emplace_back("cacertsVerified = true");
        lines.emplace_back("systemRequirementsVerified = true");
        lines.emplace_back("centralConnectionVerified = true");
        lines.emplace_back("susConnectionVerified = true");
        lines.emplace_back("cdnConnectionVerified = true");
        lines.emplace_back("installPathVerified = true");
        lines.emplace_back("installationDirIsExecutable = true");
        lines.emplace_back("registrationWithCentral = true");
        lines.emplace_back("productInstalled = true");
        lines.emplace_back("summary = successfully installed product");
        EXPECT_CALL(*fs, readLines(REPORT_FILE)).WillRepeatedly(Return(lines));
        EXPECT_CALL(*fs, readFile(REPORT_FILE)).WillRepeatedly(Return(REPORT_FILE));

        return REPORT_FILE;
    }

    class DebugConsoleSetup
    {
    public:
        DebugConsoleSetup()
        {
            Common::Logging::ConsoleLoggingSetup::consoleSetupLogging();
            log4cplus::Logger::getRoot().setLogLevel(log4cplus::DEBUG_LOG_LEVEL);
        }

        ~DebugConsoleSetup()
        {
            Common::Logging::ConsoleLoggingSetup::loggingShutdown();
        }
    };

}

TEST(Integration, send_telemetry)
{
    DebugConsoleSetup logging;

    auto mockFileSystem = std::make_unique<MockFileSystem>();

    const auto CONFIG_FILE = expectConfig(*mockFileSystem);
    const auto ARGS_FILE = expectArgs(*mockFileSystem);
    const auto REPORT_FILE = expectReport(mockFileSystem.get());

    auto platform = std::make_shared<MockPlatformUtils>();
    EXPECT_CALL(*platform, getOsName()).WillRepeatedly(Return("Test Linux"));
    EXPECT_CALL(*platform, getKernelVersion()).WillRepeatedly(Return("1.2.3"));
    EXPECT_CALL(*platform, getArchitecture()).WillRepeatedly(Return("powerpc64"));

    std::vector<std::string> args{CONFIG_FILE, ARGS_FILE, REPORT_FILE};

    auto curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
    auto httpRequester = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);

    Telemetry telemetry{args, httpRequester};
    int ret = telemetry.run(mockFileSystem.get(), *platform);
    EXPECT_EQ(ret, 0);
    auto json = telemetry.json();
    auto url = telemetry.url();

    std::ofstream of{"/tmp/telemetry.json"};
    of << json;
    of.close();

    PRINT("curl -H \"Content-Type: application/json\" -v -X PUT -T /tmp/telemtry.json " << url);
    PRINT("curl -H Expect: -H \"Content-Type: application/json\" -v -X PUT -T /tmp/telemtry.json " << url);
}
