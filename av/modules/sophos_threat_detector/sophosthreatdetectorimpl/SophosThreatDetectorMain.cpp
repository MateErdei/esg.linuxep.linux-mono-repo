/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosThreatDetectorMain.h"
#include "Logger.h"

#include "common/Define.h"
#ifdef USE_SUSI
#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>
#else
#include <sophos_threat_detector/threat_scanner/FakeSusiScannerFactory.h>
#endif
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <datatypes/Print.h>
#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <string>
#include <unistd.h>

using namespace sspl::sophosthreatdetectorimpl;
namespace fs = sophos_filesystem;

static int inner_main()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    fs::path chrootPath = pluginInstall / "chroot";
    LOGDEBUG("Preparing to enter chroot at: " << chrootPath);
#ifdef USE_CHROOT
    // Copy logger config from base
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    LOGDEBUG("SSPL installed to: " << sophosInstall);
    fs::path loggerConf = sophosInstall / "base/etc/logger.conf";
    fs::path targetFile = chrootPath / sophosInstall;
    targetFile /= "base/etc/logger.conf";
    LOGINFO("Copying: " << loggerConf << " to: " << targetFile);
    fs::copy_file(loggerConf, targetFile, fs::copy_options::overwrite_existing);

    int ret = ::chroot(chrootPath.c_str());
    if (ret != 0)
    {
        LOGERROR("Failed to chroot to " << chrootPath);
        exit(EXIT_FAILURE);
    }

    fs::path scanningSocketPath = "/scanning_socket";
#else
    fs::path scanningSocketPath = chrootPath / "scanning_socket";
#endif

    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
#ifdef USE_SUSI
        = std::make_shared<threat_scanner::SusiScannerFactory>();
#else
        = std::make_shared<threat_scanner::FakeSusiScannerFactory>();
#endif
    unixsocket::ScanningServerSocket server(scanningSocketPath, scannerFactory);
    server.run();

    return 0;
}

int sspl::sophosthreatdetectorimpl::sophos_threat_detector_main()
{
    try
    {
        return inner_main();
    }
    catch (std::exception& ex)
    {
        LOGERROR("Caught std::exception: "<<ex.what() << " at top level");
        return 101;
    }
    catch(...)
    {
        LOGERROR("Caught exception at top-level");
        return 100;
    }
}
