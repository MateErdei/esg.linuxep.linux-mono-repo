/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosThreatDetectorMain.h"
#include "Logger.h"
#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <datatypes/Print.h>

#include <string>
#include <unistd.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define CHROOT "/opt/sophos-spl/plugins/av/chroot"

using namespace sspl::sophosthreatdetectorimpl;

class MessageCallbacks : public IMessageCallback
{
    void processMessage(const std::string& message) override
    {
        LOGTRACE("scanning: " << message);
    }
};


static int inner_main()
{
    int ret;

    ret = ::chroot(CHROOT);
    if (ret != 0)
    {
        handle_error("Failed to chroot to "
                             CHROOT);
    }

    const std::string path = "/unix_socket";

    std::shared_ptr<IMessageCallback> callback = std::make_shared<MessageCallbacks>();
    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
        = std::make_shared<threat_scanner::SusiScannerFactory>();
    unixsocket::ScanningServerSocket server(path, callback, scannerFactory);
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
