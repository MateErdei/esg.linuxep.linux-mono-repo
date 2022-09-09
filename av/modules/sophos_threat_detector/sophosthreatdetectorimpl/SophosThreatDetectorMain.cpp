// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SophosThreatDetectorMain.h"

#include "Logger.h"
#include "Reloader.h"
#include "ShutdownTimer.h"
#include "ThreatReporter.h"

#include "common/Define.h"
#include "common/FDUtils.h"
#include "common/PidLockFile.h"
#include "common/SaferStrerror.h"
#include "common/signals/SigTermMonitor.h"
#include "common/signals/SigUSR1Monitor.h"

#ifdef USE_SUSI
#include <sophos_threat_detector/threat_scanner/SusiScannerFactory.h>
#else
#include <sophos_threat_detector/threat_scanner/FakeSusiScannerFactory.h>
#endif
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"
#include "unixsocket/threatDetectorSocket/ScanningServerSocket.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#define BOOST_LOCALE_HIDE_AUTO_PTR
#include <boost/locale.hpp>

#include <csignal>
#include <fstream>
#include <string>

#include <netdb.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <zlib.h>

namespace sspl::sophosthreatdetectorimpl
{
    namespace fs = sophos_filesystem;

    namespace
    {
        void attempt_dns_query()
        {
            struct addrinfo* result { nullptr };

            struct addrinfo hints
            {
            };
            hints.ai_family = PF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags |= AI_CANONNAME; // NOLINT(hicpp-signed-bitwise)

            /* resolve the domain name into a list of addresses */
            int error = getaddrinfo("4.sophosxl.net", nullptr, &hints, &result);
            if (error != 0)
            {
                if (error == EAI_SYSTEM)
                {
                    LOGERROR("Failed DNS query of 4.sophosxl.net: system error in getaddrinfo: " << common::safer_strerror(errno));
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

        void copy_etc_file_if_present(const fs::path& etcDest, const fs::path& etcSrcFile)
        {
            fs::path targetFile = etcDest;
            targetFile /= etcSrcFile.filename();

            LOGSUPPORT("Copying " << etcSrcFile << " to: " << targetFile);
            std::error_code ec;
            fs::copy_file(etcSrcFile, targetFile, fs::copy_options::overwrite_existing, ec);

            if (ec)
            {
                LOGINFO("Failed to copy " << etcSrcFile << ": " << ec.message());
            }
        }

        void copy_etc_files_for_dns(const fs::path& chrootPath)
        {
            fs::path etcDest = chrootPath;
            etcDest /= "etc";

            const std::vector<fs::path> fileVector {
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
                if (line.rfind("COVFILE=", 0) == 0)
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

        void copyRequiredFiles(const fs::path& sophosInstall, const fs::path& chrootPath)
        {
            const std::vector<fs::path> fileVector {
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
                 * sourceFile is an absolute path (/opt/sophos-spl/base/etc/logger.conf), so when /= is used
                 * the left value is replaced. chrootPath is "/opt/sophos-spl/plugins/av/chroot" so
                 * targetFile += sourceFile -> /opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/base/etc/logger.conf
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

        template<typename T, typename D, D Deleter>
        struct stateless_deleter
        {
            typedef T pointer;

            void operator()(T x)
            {
                Deleter(x);
            }
        };

        int dropCapabilities()
        {
            int ret = -1;
            std::unique_ptr<cap_t, stateless_deleter<cap_t, int (*)(void*), &cap_free>> capHandle(cap_get_proc());
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

        int lockCapabilities()
        {
            int ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
            if (ret != 0)
            {
                LOGERROR("Failed to lock capabilities: " << common::safer_strerror(errno));
            }
            return ret;
        }

        fs::path threat_reporter_socket(const fs::path& pluginInstall)
        {
            return pluginInstall / "chroot/var/threat_report_socket";
        }

        fs::path threat_detector_config(const fs::path& pluginInstall)
        {
            return pluginInstall / "chroot/etc/threat_detector_config";
        }

        void remove_shutdown_notice_file(const fs::path& pluginInstall)
        {
            fs::remove(pluginInstall / "chroot/var/threat_detector_expected_shutdown");
        }

        void create_shutdown_notice_file(const fs::path& pluginInstall)
        {
            std::ofstream ofs(pluginInstall / "chroot/var/threat_detector_expected_shutdown");
            ofs.close();
        }

        class ThreatDetectorControlCallbacks : public unixsocket::IProcessControlMessageCallback
        {
        public:
            explicit ThreatDetectorControlCallbacks(SophosThreatDetectorMain& mainInstance)
                : m_mainInstance(mainInstance)
            {}

            void processControlMessage(const scan_messages::E_COMMAND_TYPE& command) override
            {
                switch (command)
                {
                    case scan_messages::E_SHUTDOWN:
                        m_mainInstance.shutdownThreatDetector();
                        break;
                    case scan_messages::E_RELOAD:
                        m_mainInstance.reloadSUSIGlobalConfiguration();
                        break;
                    default:
                        LOGWARN("Sophos On Access Process received unknown process control message");
                }
            }

        private:
            SophosThreatDetectorMain& m_mainInstance;
        };
    } // namespace

    void SophosThreatDetectorMain::shutdownThreatDetector()
    {
        LOGINFO("Sophos Threat Detector received shutdown request");
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        create_shutdown_notice_file(pluginInstall);
        std::exit(0);
    }

    void SophosThreatDetectorMain::reloadSUSIGlobalConfiguration()
    {
        //thread safe atomic bool
        if (m_scannerFactory->susiIsInitialized())
        {
            LOGINFO("Sophos Threat Detector received reload request");
            //This ends up calling SusiGlobalHandler::reload which ensures thread safety for SUSI_UpdateGlobalConfiguration
            m_reloader->reload();
        }
        else
        {
            LOGDEBUG("Skipping susi reload because susi is not initialised");
        }
    }

    int SophosThreatDetectorMain::inner_main()
    {
        common::signals::SigTermMonitor sigTermMonitor{true};

        // Ignore SIGPIPE. send*() or write() on a broken pipe will now fail with errno=EPIPE rather than crash.
        struct sigaction ignore {};
        ignore.sa_handler = SIG_IGN;
        ::sigaction(SIGPIPE, &ignore, nullptr);

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

        if (getuid() != 0)
        {
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
        }
        else
        {
            LOGINFO("Running as root - Skip dropping of capabilities");
        }

        ret = ::chdir("/");
        if (ret != 0)
        {
            LOGERROR("Failed to chdir / after entering chroot (" << ret << ")");
            exit(EXIT_FAILURE);
        }

        fs::path scanningSocketPath = "/var/scanning_socket";
#else
        fs::path scanningSocketPath = chrootPath / "var/scanning_socket";
#endif

        remove_shutdown_notice_file(pluginInstall);
        fs::path lockfile = pluginInstall / "chroot/var/threat_detector.pid";
        common::PidLockFile lock(lockfile);

        threat_scanner::IThreatReporterSharedPtr threatReporter =
            std::make_shared<sspl::sophosthreatdetectorimpl::ThreatReporter>(threat_reporter_socket(pluginInstall));

        threat_scanner::IScanNotificationSharedPtr shutdownTimer =
            std::make_shared<ShutdownTimer>(threat_detector_config(pluginInstall));

        m_scannerFactory =
            std::make_shared<threat_scanner::SusiScannerFactory>(threatReporter, shutdownTimer);

        if (sigTermMonitor.triggered())
        {
            LOGINFO("Sophos Threat Detector received SIGTERM - shutting down");
            return common::E_CLEAN_SUCCESS;
        }
        m_scannerFactory->update(); // always force an update during start-up

        m_reloader = std::make_shared<Reloader>(m_scannerFactory);

        unixsocket::ScanningServerSocket server(scanningSocketPath, 0666, m_scannerFactory);
        server.start();

        LOGINFO("Starting USR1 monitor");
        common::signals::SigUSR1Monitor usr1Monitor(m_reloader); // Create monitor before loading SUSI

        //Always create processController after m_reloader is initialized
        fs::path processControllerSocketPath = "/var/process_control_socket";
        std::shared_ptr<ThreatDetectorControlCallbacks> callbacks = std::make_shared<ThreatDetectorControlCallbacks>(*this);
        unixsocket::ProcessControllerServerSocket processController(processControllerSocketPath, 0660, callbacks);
        processController.start();

        int returnCode = common::E_CLEAN_SUCCESS;

        fd_set readFDs;
        FD_ZERO(&readFDs);
        int max = -1;

        max = FDUtils::addFD(&readFDs, sigTermMonitor.monitorFd(), max);
        max = FDUtils::addFD(&readFDs, usr1Monitor.monitorFd(), max);

        while (true)
        {
            fd_set tempRead = readFDs;

            struct timespec timeout {};
            timeout.tv_sec = shutdownTimer->timeout();
            LOGDEBUG("Setting shutdown timeout to " << timeout.tv_sec << " seconds");
            // wait for an activity on one of the sockets
            int activity = ::pselect(max + 1, &tempRead, nullptr, nullptr, &timeout, nullptr);

            if (activity < 0)
            {
                // error in pselect
                int error = errno;
                if (error == EINTR)
                {
                    LOGDEBUG("Ignoring EINTR from pselect");
                    continue;
                }

                LOGERROR("Failed to read from socket - shutting down. Error: "
                         << common::safer_strerror(error) << " (" << error << ')');
                returnCode = common::E_GENERIC_FAILURE;
                break;
            }
            else if (activity == 0)
            {
                // no activity on the fds, must be a timeout
                if (m_scannerFactory->susiIsInitialized())
                {
                    long currentTimeout = shutdownTimer->timeout();
                    if (currentTimeout <= 0)
                    {
                        LOGDEBUG("No scans requested for " << timeout.tv_sec << " seconds - shutting down.");
                        create_shutdown_notice_file(pluginInstall);
                        break;
                    }
                    else
                    {
                        LOGDEBUG("Scan requested less than " << timeout.tv_sec << " seconds ago - continuing");
                        continue;
                    }
                }
                else
                {
                    LOGDEBUG("SUSI is not initialised - resetting shutdown timer");
                    shutdownTimer->reset();
                    continue;
                }
            }

            if (FDUtils::fd_isset(sigTermMonitor.monitorFd(), &tempRead))
            {
                LOGINFO("Sophos Threat Detector received SIGTERM - shutting down");
                sigTermMonitor.triggered();
                returnCode = common::E_CLEAN_SUCCESS;
                break;
            }

            if (FDUtils::fd_isset(usr1Monitor.monitorFd(), &tempRead))
            {
                LOGINFO("Sophos Threat Detector received SIGUSR1 - reloading");
                usr1Monitor.triggered();
            }
        }

        m_scannerFactory->shutdown();

        LOGINFO("Sophos Threat Detector is exiting with return code " << returnCode);
        return returnCode;
    }

    int SophosThreatDetectorMain::sophos_threat_detector_main()
    {
        try
        {
            return inner_main();
        }
        catch (std::exception& ex)
        {
            LOGERROR("Caught std::exception: " << ex.what() << " at top level");
            return 101;
        }
        catch (...)
        {
            LOGERROR("Caught unknown exception at top-level");
            return 100;
        }
    }
} // namespace sspl::sophosthreatdetectorimpl