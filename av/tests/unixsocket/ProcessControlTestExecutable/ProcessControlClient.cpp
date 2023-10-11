// Copyright 2021-2023 Sophos Limited. All rights reserved.


#include "unixsocket/processControllerSocket/ProcessControllerClient.h"

#include <string>

using namespace scan_messages;

int main()
{
    const std::string path = "/tmp/fd_chroot/tmp/unix_socket";
    unixsocket::ProcessControllerClientSocket socket(path);

    scan_messages::ProcessControlSerialiser shutDownRequest(E_SHUTDOWN);

    socket.sendProcessControlRequest(shutDownRequest);

    return 0;
}