/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <scan_messages/ThreatDetected.h>

#include <Common/Logging/ConsoleLoggingSetup.h>

#include <cassert>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
    struct addrinfo* result;
    struct addrinfo* res;
    int error;

    /* resolve the domain name into a list of addresses */
    error = getaddrinfo("4.sophosxl.net", NULL, NULL, &result);
    if (error != 0) {
        if (error == EAI_SYSTEM) {
            perror("getaddrinfo");
        } else {
            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
        }
        exit(EXIT_FAILURE);
    }

    /* loop over all returned results and do inverse lookup */
    for (res = result; res != NULL; res = res->ai_next) {
        char hostname[NI_MAXHOST];
        error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0);
        if (error != 0) {
            fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
            continue;
        }
        if (*hostname != '\0')
            printf("hostname: %s\n", hostname);
    }

    freeaddrinfo(result);
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
        ::chroot(chroot);
        ::chdir("/");
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

    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
        = std::make_shared<threat_scanner::SusiScannerFactory>();

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
