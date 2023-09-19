// Copyright 2023 Sophos All rights reserved.

#include "unixsocket/threatReporterSocket/ThreatReporterServerSocket.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Exceptions/Print.h"

namespace
{
    class FakeMessageCallback : public IMessageCallback
    {
    public:
        void processMessage(scan_messages::ThreatDetected) override
        {
            PRINT("Got threat detected!");
        }
    };
}

int main()
{
    Common::Logging::ConsoleLoggingSetup logging;
    auto callback = std::make_shared<FakeMessageCallback>();
    unixsocket::ThreatReporterServerSocket socket("/tmp/threat_reporter_socket", 0700, callback);
    socket.run();
    return 0;
}
