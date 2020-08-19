/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <scan_messages/ThreatDetected.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace fs = sophos_filesystem;

static int scan(const char* filename)
{
    datatypes::AutoFd fd(::open(filename, O_RDONLY));
    assert(fd.get() >= 0);

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();

    std::string PLUGIN_INSTALL = "/opt/sophos-spl/plugins/av";

    appConfig.setData("PLUGIN_INSTALL", PLUGIN_INSTALL);
    fs::path pluginInstall = PLUGIN_INSTALL;
    fs::path chrootPath = pluginInstall / "chroot";

    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
        = std::make_shared<threat_scanner::SusiScannerFactory>();

    auto scanner = scannerFactory->createScanner(true);

    auto scanType = scan_messages::E_SCAN_TYPE_ON_DEMAND;
    auto result = scanner->scan(fd, filename, scanType, "root");
    bool clean = result.clean();
    PRINT("SCAN CLEAN:" << clean);

    decltype(scannerFactory)::weak_type weak_scanner_factory = scannerFactory;

    scanner.reset();
    scannerFactory.reset();

    PRINT("Completed reset: Scanner Factory deleted: "<<weak_scanner_factory.expired());

    return clean ? 0 : 1;
}

int main(int argc, char* argv[])
{
    static Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;

    const char* filename = "/etc/fstab";
    if (argc > 1)
    {
        filename = argv[1];
    }
    return scan(filename);
}
