/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SophosThreatDetectorMain.h"
#include "Logger.h"

#include "common/Define.h"
#ifdef USE_SUSI
#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>
#else
#include <sophos_threat_detector/threat_scanner/FakeSusiScannerFactory.h>
#endif
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <fstream>
#include <string>
#include <netdb.h>

#include <linux/securebits.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <unistd.h>

using namespace sspl::sophosthreatdetectorimpl;
namespace fs = sophos_filesystem;

static void attempt_dns_query()
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
            LOGERROR("Failed DNS query of 4.sophosxl.net: system error in getaddrinfo: " << strerror(errno));
        }
        else
        {
            LOGERROR("Failed DNS query of 4.sophosxl.net: error in getaddrinfo: " << gai_strerror(error));
        }
    }
    else
    {
        LOGINFO("Successful DNS query of 4.sophosxl.net");
        freeaddrinfo(result);
    }
}

static void copy_etc_file_if_present(const fs::path& etcDest, const fs::path& etcSrcFile)
{
    fs::path targetFile = etcDest;
    targetFile /= etcSrcFile.filename();

    LOGSUPPORT("Copying " << etcSrcFile << " to: " << targetFile);
    std::error_code ec;
    fs::copy_file(etcSrcFile, targetFile, fs::copy_options::overwrite_existing, ec);

    if (ec)
    {
        LOGINFO("Failed to copy "<< etcSrcFile << ": " << ec.message());
    }
}

static void copy_etc_files_for_dns(const fs::path& chrootPath)
{
    /*
     *
function copy_etc()
{
    local F="$1"
    [[ -f "$F" ]] || return
    cp "$F" ${CHROOT}/etc/
}

copy_etc /etc/nsswitch.conf
copy_etc /etc/resolv.conf
copy_etc /etc/ld.so.cache
copy_etc /etc/host.conf
copy_etc /etc/hosts

     */
    fs::path etcDest = chrootPath;
    etcDest /= "etc";

    copy_etc_file_if_present(etcDest, "/etc/nsswitch.conf");
    copy_etc_file_if_present(etcDest, "/etc/resolv.conf");
    copy_etc_file_if_present(etcDest, "/etc/ld.so.cache");
    copy_etc_file_if_present(etcDest, "/etc/host.conf");
    copy_etc_file_if_present(etcDest, "/etc/hosts");
}

static void copyRequiredFiles(const fs::path& sophosInstall, const fs::path& chrootPath)
{
    std::vector<fs::path> fileVector;
    fileVector.emplace_back("base/etc/logger.conf");
    fileVector.emplace_back("base/etc/machine_id.txt");
    fileVector.emplace_back("base/update/var/update_config.json");
    fileVector.emplace_back("plugins/av/VERSION.ini");

    for (const fs::path& file : fileVector)
    {
        fs::path sourceFile = sophosInstall;
        sourceFile /= file;

        fs::path targetFile = chrootPath;
        targetFile /= sourceFile;

        LOGINFO("Copying " << sourceFile << " to: " << targetFile);

        try
        {
            fs::copy_file(sourceFile, targetFile, fs::copy_options::overwrite_existing);
        }
        catch (fs::filesystem_error& e)
        {
            LOGERROR("Failed to copy: " << sourceFile);
        }
    }
}

template <typename T, typename D, D Deleter>
struct stateless_deleter
{
    typedef T pointer;

    void operator()(T x)
    {
        Deleter(x);
    }
};

static int dropCapabilities()
{
    int ret = -1;
    std::unique_ptr<cap_t, stateless_deleter<cap_t, int(*)(void*), &cap_free>> capHandle(cap_get_proc());
    if (!capHandle)
    {
        LOGERROR("Failed to get effective capabilities");
        return ret;
    }

    ret = cap_clear(capHandle.get());
    if (ret != 0)
    {
        LOGERROR("Failed to clear effective capabilities");
        return ret;
    }

    ret = cap_set_proc(capHandle.get());
    if (ret != 0)
    {
        LOGERROR("Failed to set the dropped capabilities");
        return ret;
    }

    return ret;
}

static int lockCapabilities()
{
    int ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    if (ret != 0)
    {
        LOGERROR("Failed to lock capabilities: " << strerror(errno));
    }
    return ret;
}

static int inner_main()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
    fs::path chrootPath = pluginInstall / "chroot";
    LOGDEBUG("Preparing to enter chroot at: " << chrootPath);
#ifdef USE_CHROOT
    // attempt DNS query
    attempt_dns_query();

    // Copy logger config from base
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    copyRequiredFiles(sophosInstall, chrootPath);

    // Copy etc files for DNS
    copy_etc_files_for_dns(chrootPath);

    int ret = ::chroot(chrootPath.c_str());
    if (ret != 0)
    {
        LOGERROR("Failed to chroot to " << chrootPath.c_str() << " (" << errno << "): Check permissions");
        exit(EXIT_FAILURE);
    }

    ret = dropCapabilities();
    if (ret != 0)
    {
        LOGERROR("Failed to drop capabilities after entering chroot (" << ret << ")");
        exit(EXIT_FAILURE);
    }

    ret = lockCapabilities();
    if (ret != 0)
    {
        LOGERROR("Failed to lock capabilities after entering chroot (" << ret << ")");
        exit(EXIT_FAILURE);
    }

    fs::path scanningSocketPath = "/scanning_socket";
#else
    fs::path scanningSocketPath = chrootPath / "scanning_socket";
#endif

    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
#ifdef USE_SUSI
        = std::make_shared<threat_scanner::SusiScannerFactory>();
#else
        = std::make_shared<threat_scanner::FakeSusiScannerFactory>();
#endif
    unixsocket::ScanningServerSocket server(scanningSocketPath, 0666, scannerFactory);
    server.run();

    return 0;
}

int sspl::sophosthreatdetectorimpl::sophos_threat_detector_main()
{
    try
    {
        return inner_main();
    }
    catch (std::exception& ex)
    {
        LOGERROR("Caught std::exception: "<<ex.what() << " at top level");
        return 101;
    }
    catch(...)
    {
        LOGERROR("Caught unknown exception at top-level");
        return 100;
    }
}

