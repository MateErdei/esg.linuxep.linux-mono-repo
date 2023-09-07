// Copyright 2020-2023 Sophos Limited. All rights reserved.

// Class
#include "SophosThreatDetectorMain.h"
// Package
#include "Logger.h"
#include "ProcessForceExitTimer.h"
#include "ProxySettings.h"
#include "ThreatDetectorControlCallback.h"
#include "ThreatDetectorException.h"
#include "ThreatDetectorResources.h"
#include "sophos_threat_detector/threat_scanner/ThreatScannerException.h"
// AV Plugin
#include "common/ApplicationPaths.h"
#include "common/Define.h"
#include "common/SaferStrerror.h"
#include "common/ThreadRunner.h"
#include "common/signals/SigUSR1Monitor.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/UnixSocketException.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

// SPL Base
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Exceptions/IException.h"

// C++ 3rd Party
#define BOOST_LOCALE_HIDE_AUTO_PTR
#include <boost/locale.hpp>

// C++ standard
#include <cassert>
#include <csignal>
#include <fstream>
#include <string>

// C system/standard/3rd party
#include <netdb.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <zlib.h>

using namespace std::chrono_literals;

namespace
{
    class InnerMainException : public Common::Exceptions::IException
    {
    public:
        using Common::Exceptions::IException::IException;
    };
}

namespace sspl::sophosthreatdetectorimpl
{
    namespace fs = sophos_filesystem;

    namespace
    {
        class ScannerFactoryResetter
        {
        public:
            ScannerFactoryResetter(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
                : m_scannerFactory(std::move(scannerFactory))
            {}
            ~ScannerFactoryResetter()
            {
                m_scannerFactory->shutdown();
            }
        private:
            threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;
        };

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
                "base/etc/machine_id.txt",
                "plugins/av/VERSION.ini",
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


    } // namespace

    void SophosThreatDetectorMain::attempt_dns_query()
    {
        struct addrinfo* result { nullptr };

        struct addrinfo hints
        {
        };
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags |= AI_CANONNAME; // NOLINT(hicpp-signed-bitwise)

        /* resolve the domain name into a list of addresses */
        int error = m_sysCallWrapper->getaddrinfo("4.sophosxl.net", nullptr, &hints, &result);
        if (error != 0)
        {
            if (error == EAI_SYSTEM)
            {
                LOGINFO("Failed DNS query of 4.sophosxl.net: system error in getaddrinfo: " << common::safer_strerror(errno));
            }
            else
            {
                LOGINFO("Failed DNS query of 4.sophosxl.net: error in getaddrinfo: " << gai_strerror(error));
            }
        }
        else
        {
            LOGINFO("Successful DNS query of 4.sophosxl.net");
            m_sysCallWrapper->freeaddrinfo(result);
        }
    }

    int SophosThreatDetectorMain::dropCapabilities()
    {
        int ret = -1;
        std::unique_ptr<cap_t, stateless_deleter<cap_t, int (*)(void*), &cap_free>> capHandle(m_sysCallWrapper->cap_get_proc());
        if (!capHandle)
        {
            int error = errno;
            LOGERROR("Failed to get capabilities from call to cap_get_proc: "
                     <<  error << " (" << common::safer_strerror(error) << ")");
            return ret;
        }

        ret = m_sysCallWrapper->cap_clear(capHandle.get());
        if (ret == -1)
        {
            int error = errno;
            LOGERROR("Failed to clear effective capabilities: "
                     <<  error << " (" << common::safer_strerror(error) << ")");
            return ret;
        }

        ret = m_sysCallWrapper->cap_set_proc(capHandle.get());
        if (ret == -1)
        {
            int error = errno;
            LOGERROR("Failed to set the dropped capabilities: "
                     <<  error << " (" << common::safer_strerror(error) << ")");
            return ret;
        }

        return ret;
    }

    int SophosThreatDetectorMain::lockCapabilities()
    {
        return m_sysCallWrapper->prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    }

    void SophosThreatDetectorMain::shutdownThreatDetector()
    {
        LOGINFO("Sophos Threat Detector received shutdown request");
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        create_shutdown_notice_file(pluginInstall);
        m_systemFileRestartTrigger.notify();
    }

    void SophosThreatDetectorMain::reloadSUSIGlobalConfiguration()
    {
        assert(m_reloader);
        assert(m_scannerFactory);

        // Read new SUSI settings into memory
        bool susiSettingsChanged = m_reloader->updateSusiConfig();

        // thread safe atomic bool
        if (m_scannerFactory->susiIsInitialized())
        {
            LOGINFO("Sophos Threat Detector received reload request");
            // This ends up calling SusiGlobalHandler::reload which ensures thread safety for
            // SUSI_UpdateGlobalConfiguration
            m_reloader->reload();
        }
        else
        {
            LOGDEBUG("Skipping susi reload because susi is not initialised");
        }

        if (susiSettingsChanged)
        {
            LOGINFO("Triggering rescan of SafeStore database");

            // Tell on-access to clear cached file events because they may now be invalid due to allow-list changes
            m_updateCompleteNotifier->updateComplete();

            // Then trigger the rescan
            m_safeStoreRescanWorker->triggerRescan();
        }
        else
        {
            LOGDEBUG("Not triggering SafeStore rescan because SUSI settings have not changed");
        }
    }

    int SophosThreatDetectorMain::inner_main(const IThreatDetectorResourcesSharedPtr& resources)
    {
        assert(resources);
        m_sysCallWrapper = resources->createSystemCallWrapper();

        auto processForceExitTimer = std::make_shared<ProcessForceExitTimer>(10s, m_sysCallWrapper);
        common::ThreadRunner processForceExitTimerThread(processForceExitTimer, "processForceExitTimer", false);

        auto sigTermMonitor = resources->createSigTermHandler(true);

        // Ignore SIGPIPE. send*() or write() on a broken pipe will now fail with errno=EPIPE rather than crash.
        struct sigaction ignore {};
        ignore.sa_handler = SIG_IGN;
        ::sigaction(SIGPIPE, &ignore, nullptr);

        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        fs::path pluginInstall = appConfig.getData("PLUGIN_INSTALL");
        fs::path chrootPath = pluginInstall / "chroot";


        // Load proxy settings
        setProxyFromConfigFile();


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

        int ret = m_sysCallWrapper->chroot(chrootPath.c_str());
        if (ret == -1)
        {
            int error = errno;
            std::stringstream logmsg;
            logmsg << "Failed to chroot to " << chrootPath.c_str() <<
                ": " <<  error << " (" << common::safer_strerror(error) << ")";
            throw InnerMainException(LOCATION, logmsg.str());
        }

        if (m_sysCallWrapper->getuid() != 0)
        {
            ret = dropCapabilities();
            if (ret != 0)
            {
                throw InnerMainException(LOCATION, "Failed to drop capabilities after entering chroot");
            }

            ret = lockCapabilities();
            if (ret != 0)
            {
                int error = errno;
                std::stringstream logmsg;
                logmsg << "Failed to lock capabilities after entering chroot: "
                       <<  error << " (" << common::safer_strerror(error) << ")";
                throw InnerMainException(LOCATION, logmsg.str());
            }
        }
        else
        {
            LOGINFO("Running as root - Skip dropping of capabilities");
        }

        ret = m_sysCallWrapper->chdir("/");
        if (ret == -1)
        {
            int error = errno;
            std::stringstream logmsg;
            logmsg << "Failed to chdir / after entering chroot " << error << " (" << common::safer_strerror(error) << ")";
            throw InnerMainException(LOCATION, logmsg.str());
        }

        chrootPath = "/";
#endif
        fs::path scanningSocketPath = chrootPath / "var/scanning_socket";
        fs::path updateCompletePath = chrootPath / "var/update_complete_socket";

        remove_shutdown_notice_file(pluginInstall);
        fs::path lockfile = chrootPath / "var/threat_detector.pid";
        auto pidLock = resources->createPidLockFile(lockfile);

        auto threatReporter = resources->createThreatReporter(threat_reporter_socket(pluginInstall));
        auto shutdownTimer = resources->createShutdownTimer(threat_detector_config(pluginInstall));

        m_updateCompleteNotifier = resources->createUpdateCompleteNotifier(updateCompletePath, 0700);
        common::ThreadRunner updateCompleteNotifierThread (m_updateCompleteNotifier, "updateCompleteNotifier", true);

        m_scannerFactory = resources->createSusiScannerFactory(threatReporter, shutdownTimer, m_updateCompleteNotifier);
        ScannerFactoryResetter scannerFactoryShutdown(m_scannerFactory);

        if (sigTermMonitor->triggered())
        {
            LOGINFO("Sophos Threat Detector received SIGTERM - shutting down");
            // Scanner factory shutdown by scannerFactoryShutdown
            return common::E_CLEAN_SUCCESS;
        }

        if(!m_scannerFactory->update()) // always force an update during start-up
        {
            LOGFATAL("Update of scanner at startup failed exiting threat detector main");
            return common::E_GENERIC_FAILURE;
        }

        m_reloader = resources->createReloader(m_scannerFactory);

        // process control depends on m_reloader which depends on m_scannerFactory
        fs::path processControllerSocketPath = "/var/process_control_socket";
        auto callbacks = std::make_shared<ThreatDetectorControlCallback>(*this);
        auto processController = resources->createProcessControllerServerSocket(processControllerSocketPath, 0660, callbacks);
        common::ThreadRunner processControllerSocketThread (processController, "processControllerSocket", true);

        auto usr1Monitor = resources->createUsr1Monitor(m_reloader);

        auto server = resources->createScanningServerSocket(scanningSocketPath, 0666, m_scannerFactory);
        common::ThreadRunner scanningServerSocketThread (server, "scanningServerSocket", true);

        m_safeStoreRescanWorker = resources->createSafeStoreRescanWorker(Plugin::getSafeStoreRescanSocketPath());
        common::ThreadRunner safeStoreRescanWorkerThread (m_safeStoreRescanWorker, "safestoreRescanWorker", true);

        auto metadataRescanServer =
            resources->createMetadataRescanServerSocket(Plugin::getMetadataRescanSocketPath(), 0600, m_scannerFactory);
        common::ThreadRunner metadataRescanServerSocketThread(metadataRescanServer, "metadataRescanServerSocket", true);

        m_scannerFactory->loadSusiSettingsIfRequired();

        int returnCode = common::E_CLEAN_SUCCESS;

        constexpr int SIGTERM_MONITOR = 0;
        constexpr int USR1_MONITOR = 1;
        constexpr int SYSTEM_FILE_CHANGE = 2;
        struct pollfd fds[] {
            { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
            { .fd = usr1Monitor->monitorFd(), .events = POLLIN, .revents = 0 },
            { .fd = m_systemFileRestartTrigger.readFd(), .events = POLLIN, .revents = 0 },
        };

        while (true)
        {
            struct timespec timeout {};
            timeout.tv_sec = shutdownTimer->timeout();
            LOGDEBUG("Setting shutdown timeout to " << timeout.tv_sec << " seconds");
            // wait for an activity on one of the sockets
            int activity = m_sysCallWrapper->ppoll(fds, std::size(fds), &timeout, nullptr);

            if (activity < 0)
            {
                // error in ppoll
                int error = errno;
                if (error == EINTR)
                {
                    LOGDEBUG("Ignoring EINTR from ppoll");
                    continue;
                }

                LOGERROR("Failed to poll signals. Error: "
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
                        returnCode = common::E_QUICK_RESTART_SUCCESS;
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
            if ((fds[SIGTERM_MONITOR].revents & POLLERR) != 0)
            {
                LOGFATAL("Error from sigterm pipe");
                returnCode = common::E_GENERIC_FAILURE;
                break;
            }
            if ((fds[SIGTERM_MONITOR].revents & POLLIN) != 0)
            {
                LOGINFO("Sophos Threat Detector received SIGTERM - shutting down");
                sigTermMonitor->triggered();
                returnCode = common::E_CLEAN_SUCCESS;
                break;
            }

            if ((fds[USR1_MONITOR].revents & POLLERR) != 0)
            {
                LOGFATAL("Error from USR1 monitor");
                returnCode = common::E_GENERIC_FAILURE;
                break;
            }
            if ((fds[USR1_MONITOR].revents & POLLIN) != 0)
            {
                LOGINFO("Sophos Threat Detector received SIGUSR1 - reloading");
                usr1Monitor->triggered();
                // Continue running
            }

            if ((fds[SYSTEM_FILE_CHANGE].revents & POLLERR) != 0)
            {
                LOGFATAL("Error from system file change pipe");
                returnCode = common::E_GENERIC_FAILURE;
                break;
            }
            if ((fds[SYSTEM_FILE_CHANGE].revents & POLLIN) != 0)
            {
                LOGINFO("Sophos Threat Detector is restarting to pick up changed system files");
                returnCode = common::E_QUICK_RESTART_SUCCESS;
                break;
            }
        }

        // Scanner factory shutdown by scannerFactoryShutdown
        LOGINFO("Sophos Threat Detector is exiting with return code " << returnCode);
        // Exit in 10 seconds if scanner threads not joined
        processForceExitTimer->setExitCode(returnCode);
        processForceExitTimerThread.startIfNotStarted();
        return returnCode;
    }

    int SophosThreatDetectorMain::outer_main(const IThreatDetectorResourcesSharedPtr& resources)
    {
        try
        {
            return inner_main(resources);
        }
        catch (const InnerMainException& ex)
        {
            LOGFATAL("ThreatDetectorMain: InnerMainException caught at top level: " << ex.what_with_location());
            return common::E_INNER_MAIN_EXCEPTION;
        }
        catch (const SusiUpdateFailedException& ex)
        {
            LOGFATAL("Failed to update SUSI - restarting to try to recover");
            return common::E_QUICK_RESTART_SUCCESS;
        }
        catch (const ThreatDetectorException& ex)
        {
            LOGFATAL("ThreatDetectorMain: ThreatDetectorException caught at top level: " << ex.what_with_location());
            return common::E_THREAT_DETECTOR_EXCEPTION;
        }
        catch (const threat_scanner::ThreatScannerException& ex)
        {
            LOGFATAL("ThreatDetectorMain: ThreatScannerException caught at top level: " << ex.what_with_location());
            return common::E_THREAT_SCANNER_EXCEPTION;
        }
        catch (const unixsocket::UnixSocketException& ex)
        {
            LOGFATAL("ThreatDetectorMain: UnixSocketException caught at top level: " << ex.what_with_location());
            return common::E_UNIX_SOCKET_EXCEPTION;
        }
        catch (const datatypes::AVException& ex)
        {
            LOGFATAL("ThreatDetectorMain: AVException caught at top level: " << ex.what_with_location());
            return common::E_AV_EXCEPTION;
        }
        catch (const Common::Exceptions::IException& ex)
        {
            LOGFATAL("ThreatDetectorMain: IException caught at top level: " << ex.what_with_location());
            return common::E_IEXCEPTION_AT_TOP_LEVEL;
        }
        catch (const std::runtime_error& ex)
        {
            LOGFATAL("ThreatDetectorMain: RuntimeError caught at top level: " << ex.what());
            return common::E_RUNTIME_ERROR;
        }
        catch (const std::bad_alloc& ex)
        {
            LOGFATAL("ThreatDetectorMain: bad_alloc caught at top level: " << ex.what());
            return common::E_BAD_ALLOC;
        }
        catch (const std::exception& ex)
        {
            LOGFATAL("ThreatDetectorMain, Exception caught at top level: " << ex.what());
            return common::E_STD_EXCEPTION_AT_TOP_LEVEL;
        }
        catch (...)
        {
            LOGFATAL("ThreatDetectorMain, Non-std::exception caught at top-level");
            return common::E_NON_EXCEPTION_AT_TOP_LEVEL;
        }
    }

    int SophosThreatDetectorMain::sophos_threat_detector_main()
    {
        LOGINFO("Sophos Threat Detectors " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << "started");
        auto resources = std::make_shared<ThreatDetectorResources>();
        return outer_main(std::move(resources));
    }

} // namespace sspl::sophosthreatdetectorimpl