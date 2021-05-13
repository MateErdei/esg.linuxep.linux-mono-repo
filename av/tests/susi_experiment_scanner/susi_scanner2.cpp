/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_threat_detector/threat_scanner/SusiScannerFactory.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <scan_messages/ThreatDetected.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <cassert>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifndef   NI_MAXHOST
#define   NI_MAXHOST 1025
#endif

namespace fs = sophos_filesystem;

static std::string getPluginInstall()
{
    const char* PLUGIN_INSTALL = getenv("PLUGIN_INSTALL");
    if (PLUGIN_INSTALL != nullptr)
    {
        return PLUGIN_INSTALL;
    }

    const char* BASE = getenv("BASE");
    if (BASE != nullptr)
    {
        return BASE;
    }
    const char* cwd_str = getcwd(nullptr, 0); // Linux extension to allocate space
    fs::path cwd(cwd_str);
    fs::path chroot = cwd / "chroot";
    if (fs::is_directory(chroot))
    {
        return cwd;
    }

    return "/opt/sophos-spl/plugins/av";
}

static void attempt_dns()
{
    struct addrinfo* result{nullptr};

    struct addrinfo hints{};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    /* resolve the domain name into a list of addresses */
    int error = getaddrinfo("4.sophosxl.net", nullptr, &hints, &result);
    if (error != 0)
    {
        if (error == EAI_SYSTEM)
        {
            perror("getaddrinfo");
        } else {
            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
        }
        exit(EXIT_FAILURE);
    }

    char* canon_name = nullptr;

    for (struct addrinfo* res = result; res != nullptr; res = res->ai_next)
    {
        if (canon_name == nullptr)
        {
            canon_name = res->ai_canonname;
        }
        assert (canon_name != nullptr);
        void* ptr = nullptr;
        switch (res->ai_family)
        {
            case AF_INET:
                ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
                break;
            case AF_INET6:
                ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
                break;
        }
        if (ptr != nullptr)
        {
            char addrstr[255];
            inet_ntop(res->ai_family, ptr, addrstr, sizeof(addrstr));

            printf("IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4, addrstr, canon_name);
        }
    }

    freeaddrinfo(result);
}

namespace
{
    class FakeThreatReporter : public threat_scanner::IThreatReporter
    {
    public:

        void sendThreatReport(
            const std::string&,
            const std::string&,
            int64_t,
            const std::string&,
            std::time_t) override
        {
                PRINT("Reporting threat");
        };
    };

    class FakeShutdownTimer : public threat_scanner::IScanNotification
    {
    public:
        void reset() override
        {
            PRINT("Starting shutdown timer");
        };

        long timeout() override
        {
            return 0;
        }
    };
}

static int scan(const char* filename, const char* chroot)
{
    PRINT("Scanning "<< filename);
    datatypes::AutoFd fd(::open(filename, O_RDONLY));
    assert(fd.get() >= 0);

    // Do DNS lookup for 4.sophosxl.net
    attempt_dns();

    if (chroot != nullptr)
    {
        PRINT("chroot: "<< chroot);
        int ret = ::chroot(chroot);
        if (ret != 0)
        {
            perror("Failed to chroot");
            return 10;
        }
        ret = ::chdir("/");
        if (ret != 0)
        {
            perror("Failed to cd /");
            return 11;
        }
    }
    else
    {
        PRINT("NOT CHROOTing");
    }

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();

    std::string PLUGIN_INSTALL = getPluginInstall();

    appConfig.setData("PLUGIN_INSTALL", PLUGIN_INSTALL);
    fs::path pluginInstall = PLUGIN_INSTALL;
    fs::path chrootPath = pluginInstall / "chroot";

    auto threatReporter = std::make_shared<FakeThreatReporter>();
    auto shutdownTimer = std::make_shared<FakeShutdownTimer>();

    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
        = std::make_shared<threat_scanner::SusiScannerFactory>(threatReporter, shutdownTimer);

    auto scanner = scannerFactory->createScanner(true);

    auto scanType = scan_messages::E_SCAN_TYPE_ON_DEMAND;
    auto result = scanner->scan(fd, filename, scanType, "root");
    bool clean = result.allClean();
    PRINT("SCAN of "<< filename << " CLEAN=" << clean);

    decltype(scannerFactory)::weak_type weak_scanner_factory = scannerFactory;

    scanner.reset();
    scannerFactory.reset();

    PRINT("Completed reset: Scanner Factory deleted: "<<weak_scanner_factory.expired());

    return clean ? 0 : 1;
}

int main(int argc, char* argv[])
{
    static Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;

    const char* filename = "/etc/fstab";
    if (argc > 1)
    {
        filename = argv[1];
    }
    const char* chroot = (argc > 2) ? argv[2] : nullptr;
    return scan(filename, chroot);
}
