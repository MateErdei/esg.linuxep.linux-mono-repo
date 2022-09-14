// Copyright 2022, Sophos Limited.  All rights reserved.

#include "unixsocket/updateCompleteSocket/UpdateCompleteClientSocketThread.h"

#include <iostream>

using namespace threat_scanner;
using namespace unixsocket::updateCompleteSocket;

namespace
{
    class UpdateCompleteCallback : public IUpdateCompleteCallback
    {
    public:
        void updateComplete() override
        {
            std::cout << "IDE update completed!\n";
        }
    };
}

int main()
{
    auto callback = std::make_shared<UpdateCompleteCallback>();

    const auto* path = "/opt/sophos-spl/plugins/av/chroot/var/update_complete_socket";

    auto socket = std::make_shared<UpdateCompleteClientSocketThread>(path, callback);
    socket->start();
    socket->join();
    return 0;
}
