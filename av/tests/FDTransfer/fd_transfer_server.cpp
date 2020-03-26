/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

//
// Created by Douglas Leeder on 19/12/2019.
//

#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "datatypes/Print.h"
#include <string>
#include <unistd.h>
#include <sys/stat.h>


#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
namespace
{
}

int main()
{
    int ret = ::mkdir("/tmp/fd_chroot", 0700);
    static_cast<void>(ret); // ignore

    ret = chroot("/tmp/fd_chroot");
    if (ret != 0)
    {
        handle_error("Failed to chroot to /tmp/fd_chroot");
    }

    ret = ::mkdir("/tmp", 0700);
    static_cast<void>(ret); // ignore

    const std::string path = "/tmp/unix_socket";
    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
            = std::make_shared<threat_scanner::SusiScannerFactory>();
    unixsocket::ScanningServerSocket server(path, scannerFactory);
    server.run();

    return 0;
}
