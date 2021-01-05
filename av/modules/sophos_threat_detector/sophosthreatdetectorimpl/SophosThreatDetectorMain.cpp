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
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#define BOOST_LOCALE_HIDE_AUTO_PTR
#include <boost/locale.hpp>
#include <linux/securebits.h>
#include <unixsocket/threatDetectorSocket/Reloader.h>
#include <unixsocket/threatDetectorSocket/SigUSR1Monitor.h>

#include <fstream>
#include <string>

#include <netdb.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <zlib.h>

using namespace sspl::sophosthreatdetectorimpl;
namespace fs = sophos_filesystem;

static void attempt_dns_query()
{
    struct addrinfo* result{nullptr};

    struct addrinfo hints{};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME; // NOLINT(hicpp-signed-bitwise)

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
    fs::path etcDest = chrootPath;
    etcDest /= "etc";

    const std::vector<fs::path> fileVector
    {
        "/etc/nsswitch.conf",
        "/etc/resolv.conf",
        "/etc/ld.so.cache",
        "/etc/host.conf",
        "/etc/hosts",
    };

    for (const fs::path& file : fileVector)
    {
        copy_etc_file_if_present(etcDest, file);
    }
}


#if _BullseyeCoverage
#pragma BullseyeCoverage save off
static void copy_bullseye_files(const fs::path& chrootPath)
{
    LOGINFO("Setting up chroot for Bullseye coverage");

    const fs::path envPath = "/tmp/BullseyeCoverageEnv.txt";

    fs::path targetFile = chrootPath;
    targetFile += envPath; // use += rather than /= to append an absolute path
    try
    {
        fs::copy_file(envPath, targetFile, fs::copy_options::overwrite_existing);
    }
    catch (fs::filesystem_error& e)
    {
        LOGERROR("Failed to copy: " << envPath);
    }

    // find path to covfile, with fallback/default
    fs::path covFile = "/tmp/sspl-plugin-av-robot.cov";
    std::ifstream envFile(envPath);
    std::string line;
    while (std::getline(envFile, line))
    {
        if(line.rfind("COVFILE=", 0) == 0)
        {
            covFile = line.substr(8);
            LOGINFO("Using covfile: " << covFile);
            break;
        }
    }
    envFile.close();

    targetFile = chrootPath;
    targetFile += covFile; // use += rather than /= to append an absolute path
    try
    {
        if (!fs::exists(targetFile))
        {
            fs::create_hard_link(covFile, targetFile);
        }
    }
    catch (fs::filesystem_error& e)
    {
        LOGERROR("Failed to link: " << covFile << " (" << e.code() << ": " << e.what() << ")");
    }
}
#pragma BullseyeCoverage restore
#endif

static void copyRequiredFiles(const fs::path& sophosInstall, const fs::path& chrootPath)
{
    const std::vector<fs::path> fileVector
    {
        "base/etc/logger.conf",
        "base/etc/machine_id.txt",
        "plugins/av/VERSION.ini"
    };

    for (const fs::path& file : fileVector)
    {
        fs::path sourceFile = sophosInstall;
        if (file.is_absolute())
        {
            sourceFile = file;
        }
        else
        {
            sourceFile /= file;
        }

        fs::path targetFile = chrootPath;
        /*
         * sourceFile is an absolute path (/opt/sophos-spl/base/etc/logger.conf), so when /= is used the left value is replaced.
         * chrootPath is "/opt/sophos-spl/plugins/av/chroot"
         * so targetFile += sourceFile -> /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/logger.conf
         */
        targetFile += sourceFile;

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

    // ensure zlib library is loaded
    (void) zlibVersion();

    // ensure all charsets are loaded from boost::locale before entering chroot
    boost::locale::generator gen;
    std::locale localeEUC_JP = gen("EUC-JP");
    boost::locale::conv::to_utf<char>("", localeEUC_JP);
    std::locale localeShift_JIS = gen("Shift-JIS");
    boost::locale::conv::to_utf<char>("", localeShift_JIS);
    std::locale localeSJIS = gen("SJIS");
    boost::locale::conv::to_utf<char>("", localeSJIS);
    std::locale localeLatin1 = gen("Latin1");
    boost::locale::conv::to_utf<char>("", localeLatin1);

    // Copy logger config from base
    fs::path sophosInstall = appConfig.getData("SOPHOS_INSTALL");
    copyRequiredFiles(sophosInstall, chrootPath);

    // Copy etc files for DNS
    copy_etc_files_for_dns(chrootPath);

#if _BullseyeCoverage
#pragma BullseyeCoverage save off
    copy_bullseye_files(chrootPath);
#pragma BullseyeCoverage restore
#endif

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

    fs::path scanningSocketPath = "/var/scanning_socket";
#else
    fs::path scanningSocketPath = chrootPath / "var/scanning_socket";
#endif

    auto reloader = std::make_shared<unixsocket::Reloader>();
    auto usr1Monitor =  std::make_shared<unixsocket::SigUSR1Monitor>(reloader); // Create monitor before loading SUSI

    threat_scanner::IThreatScannerFactorySharedPtr scannerFactory
#ifdef USE_SUSI
        = std::make_shared<threat_scanner::SusiScannerFactory>();
#else
        = std::make_shared<threat_scanner::FakeSusiScannerFactory>();
#endif

    scannerFactory->update(); // always force an update during start-up
    reloader->reset(scannerFactory); // Actually reset the scannerFactory
    reloader.reset();

    unixsocket::ScanningServerSocket server(scanningSocketPath, 0666, scannerFactory, std::move(usr1Monitor));
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

