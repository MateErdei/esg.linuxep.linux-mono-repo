/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

//
// Created by Douglas Leeder on 19/12/2019.
//

#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "datatypes/Print.h"
#include <string>
#include <unistd.h>
#include <sys/stat.h>


#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
namespace
{
    class FakeScanner : public threat_scanner::IThreatScanner
    {
        scan_messages::ScanResponse scan(
            datatypes::AutoFd&,
            const std::string& file_path,
            int64_t /*scanType*/,
            const std::string& /*userID*/) override
        {
            PRINT(file_path);
            scan_messages::ScanResponse response;
            response.addDetection("/bin/bash", "","");
            return response;
        }
    };
    class FakeScannerFactory : public threat_scanner::IThreatScannerFactory
    {
        threat_scanner::IThreatScannerPtr createScanner(bool, bool) override
        {
            return std::make_unique<FakeScanner>();
        }

        bool update() override
        {
            return true;
        }

        ReloadResult reload() override
        {
            ReloadResult result;
            result.success = true;
            result.allowListChanged = true;
            return result;
        }

        void shutdown() override
        {
        }

        bool susiIsInitialized() override
        {
            return true;
        }

        bool hasConfigChanged() override
        {
            return false;
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
