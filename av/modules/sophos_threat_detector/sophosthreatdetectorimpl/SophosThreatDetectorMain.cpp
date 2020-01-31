/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosThreatDetectorMain.h"
#include "Logger.h"

#include "unixsocket/ScanningServerSocket.h"

#include <string>
#include <unistd.h>
#include <datatypes/Print.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define CHROOT "/opt/sophos-spl/plugins/av/chroot"

using namespace sspl::sophosthreatdetectorimpl;


static int inner_main()
{
    int ret;

    ret = chroot(CHROOT);
    if (ret != 0)
    {
        handle_error("Failed to chroot to "
                             CHROOT);
    }

    const std::string path = "/unix_socket";

    unixsocket::ScanningServerSocket server(path);
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
