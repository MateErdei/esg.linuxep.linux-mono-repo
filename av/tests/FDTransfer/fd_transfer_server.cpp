// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "tests/common/FakeThreatScannerFactory.h"

#include "datatypes/Print.h"
#include <string>
#include <unistd.h>
#include <sys/stat.h>


#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
namespace
{
    class FakeScanner : public threat_scanner::IThreatScanner
    {
        scan_messages::ScanResponse scan(datatypes::AutoFd&, const scan_messages::ScanRequest& info) override
        {
            PRINT(info.getPath());
            scan_messages::ScanResponse response;
            response.addDetection("/bin/bash", "", "", "");
            return response;
        }

        scan_messages::MetadataRescanResponse metadataRescan(const scan_messages::MetadataRescan&) override
        {
            return scan_messages::MetadataRescanResponse::clean;
        }
    };
    class FakeScannerFactory : public FakeThreatScannerFactory
    {
        threat_scanner::IThreatScannerPtr createScanner(bool, bool, bool) override
        {
            return std::make_unique<FakeScanner>();
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
    auto scannerFactory = std::make_shared<FakeScannerFactory>();
    unixsocket::ScanningServerSocket server(path, 0666, scannerFactory);
    server.run();

    return 0;
}
