// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "datatypes/Print.h"
#include "sophos_threat_detector/threat_scanner/IThreatScannerFactory.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)
namespace
{
    class FakeScanner : public threat_scanner::IThreatScanner
    {
        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const scan_messages::ScanRequest& info) override
        {
            for (int i = 0; i < 1000; i++)
            {
                unsigned oldflags = ::fcntl(fd.get(), F_GETFL, 0);
                if ((oldflags & O_DIRECT) > 0)
                {
                    PRINT(info.getPath() << " is O_DIRECT");
                }
                else
                {
                    PRINT(info.getPath() << " is NOT O_DIRECT");
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            fd.close();

            scan_messages::ScanResponse response;
            response.addDetection("/bin/bash", "", "", "");
            return response;
        }

        scan_messages::MetadataRescanResponse metadataRescan(const scan_messages::MetadataRescan&) override
        {
            return scan_messages::MetadataRescanResponse::clean;
        }
    };
    class FakeScannerFactory : public threat_scanner::IThreatScannerFactory
    {
        threat_scanner::IThreatScannerPtr createScanner(bool, bool, bool) override
        {
            return std::make_unique<FakeScanner>();
        }

        bool update() override
        {
            return true;
        }

        bool reload() override
        {
           return true;
        }

        void shutdown() override
        {
        }

        bool susiIsInitialized() override
        {
            return true;
        }

        bool updateSusiConfig() override
        {
            return false;
        }

        bool detectPUAsEnabled() override
        {
            return true;
        }
    };
}

int main()
{
    static Common::Logging::ConsoleLoggingSetup m_loggingSetup;

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
