// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "unixsocket/threatReporterSocket/ThreatReporterClient.h"

#include "tests/scan_messages/SampleThreatDetected.h"

int main()
{
    unixsocket::ThreatReporterClientSocket socket("/tmp/fd_chroot/tmp/unix_socket");
    socket.sendThreatDetection(createThreatDetectedWithRealFd({}));

    return 0;
}
