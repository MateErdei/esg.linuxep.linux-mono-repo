/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include <string>
#include <unistd.h>
#include <sys/stat.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

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

    unixsocket::ThreatReporterServerSocket server(path);
    server.run();

    return 0;
}
