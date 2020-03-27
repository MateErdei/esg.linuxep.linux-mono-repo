/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosThreatDetectorMain.h"
#include "Logger.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "common/Define.h"
#include <datatypes/Print.h>
#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <string>
#include <unistd.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

using namespace sspl::sophosthreatdetectorimpl;
namespace fs = sophos_filesystem;

class MessageCallbacks : public IMessageCallback
{
    void processMessage(const std::string& message) override
    {
        LOGTRACE("scanning: " << message);
    }
};


static int inner_main()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    fs::path chrootPath = pluginInstall / "chroot";
#ifdef USE_CHROOT
    int ret;

    ret = ::chroot(chrootPath.string());
    if (ret != 0)
    {
        handle_error("Failed to chroot to " chrootPath);
    }

    fs::path scanningSocketPath = "/scanning_socket";
#else
    fs::path scanningSocketPath = chrootPath / "scanning_socket";
#endif

    std::shared_ptr<IMessageCallback> callback = std::make_shared<MessageCallbacks>();
    unixsocket::ScanningServerSocket server(scanningSocketPath, callback);
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
