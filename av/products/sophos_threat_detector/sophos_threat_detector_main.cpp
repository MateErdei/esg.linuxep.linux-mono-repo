/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "unixsocket/ScanningServerSocket.h"

#include <string>
#include <unistd.h>
#include <sys/stat.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

#define CHROOT "/opt/sophos-spl/plugins/av/chroot"

int main()
{
    int ret;

    ret = chroot(CHROOT);
    if (ret != 0)
    {
        handle_error("Failed to chroot to " CHROOT);
    }

    const std::string path = "/unix_socket";

    unixsocket::ScanningServerSocket server(path);
    server.run();

    return 0;
}
