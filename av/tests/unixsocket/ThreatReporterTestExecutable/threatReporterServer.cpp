/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"
#include "datatypes/Print.h"
#include <string>
#include <unistd.h>
#include <sys/stat.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

namespace
{
    class MessageCallbacks : public IMessageCallback
    {
    public:
        void processMessage(const std::string& message) override
        {
            PRINT(message);
        }
    };
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

    std::shared_ptr<IMessageCallback> callback = std::make_shared<MessageCallbacks>();
    unixsocket::ThreatReporterServerSocket server(path, 0600, callback);
    server.run();

    return 0;
}
