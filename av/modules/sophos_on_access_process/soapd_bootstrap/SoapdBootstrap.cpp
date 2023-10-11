// Copyright 2022-2023 Sophos Limited. All rights reserved.

// Class
#include "SoapdBootstrap.h"
// Package
#include "Logger.h"
#include "OnAccessProcesControlCallbacks.h"
#include "SoapdResources.h"
// Product
#include "common/PidLockFile.h"
#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "common/signals/SigIntMonitor.h"
#include "common/signals/SigTermMonitor.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

// Std C++
#include <memory>
// Std C
#include <poll.h>

namespace fs = sophos_filesystem;

using namespace sophos_on_access_process::soapd_bootstrap;
using namespace sophos_on_access_process::OnAccessConfig;
using namespace onaccessimpl::onaccesstelemetry;

SoapdBootstrap::SoapdBootstrap(ISoapdResources& soapdResources, std::shared_ptr<IOnAccessRunner> onAccessRunner)
    : m_soapdResources(soapdResources)
    , m_onAccessRunner(onAccessRunner)
{
}

int SoapdBootstrap::runSoapd()
{
    SoapdResources soapdResources;
    auto sysCallWrapper = soapdResources.getSystemCallWrapper();
    auto serviceImpl = soapdResources.getOnAccessServiceImpl();
    auto TelemetryUtility = serviceImpl->getTelemetryUtility();
    auto onAccessRunner = soapdResources.getOnAccessRunner(sysCallWrapper, TelemetryUtility);
    auto SoapdInstance = SoapdBootstrap(soapdResources, onAccessRunner);
    return SoapdInstance.outerRun();
}

int SoapdBootstrap::outerRun()
{
    LOGINFO("Sophos on access process " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");

    try
    {
        innerRun();
    }
    catch (const std::exception& e)
    {
        LOGFATAL("Exception caught at top-level: " << e.what());
        return 1;
    }
    catch (...)
    {
        LOGFATAL("Non-std::exception caught at top-level");
        return 2;
    }

    LOGINFO("Exiting soapd");
    return 0;
}

void SoapdBootstrap::innerRun()
{
    m_sysCallWrapper = m_soapdResources.getSystemCallWrapper();

    // Take soapd lock file
    fs::path lockfile = common::getPluginInstallPath() / "var/soapd.pid";
    auto lock =m_soapdResources.getPidLockFile(lockfile, true);

    auto sigIntMonitor{common::signals::SigIntMonitor::getSigIntMonitor(true)};
    auto sigTermMonitor{common::signals::SigTermMonitor::getSigTermMonitor(true)};

    m_onAccessRunner->setupOnAccess();

    fs::path updateSocketPath = common::getPluginInstallPath() / "chroot/var/update_complete_socket";
    auto updateCompleteCallback = m_onAccessRunner->getUpdateCompleteCallbackPtr();
    auto updateClient = m_soapdResources.getUpdateClient(updateSocketPath, updateCompleteCallback);
    auto updateClientThread = std::make_unique<common::ThreadRunner>(updateClient, "updateClient", true);

    fs::path socketPath = common::getPluginInstallPath() / "var/soapd_controller";
    LOGDEBUG("Control Server Socket is at: " << socketPath);
    auto processControlCallbacks = std::make_shared<OnAccessProcessControlCallback>(*m_onAccessRunner);
    auto processControllerServer = m_soapdResources.getProcessController(
            socketPath, "sophos-spl-av", "sophos-spl-group", 0600, processControlCallbacks);
    auto processControllerServerThread =
        std::make_unique<common::ThreadRunner>(processControllerServer, "processControlServer", true);

    m_onAccessRunner->ProcessPolicy();

    struct pollfd fds[] {
        { .fd = sigIntMonitor->monitorFd(), .events = POLLIN, .revents = 0 },
        { .fd = sigTermMonitor->monitorFd(), .events = POLLIN, .revents = 0 }
    };

    while (true)
    {
        timespec* timeout = m_onAccessRunner->getTimeout();

        // wait for an activity on one of the fds
        int activity = m_sysCallWrapper->ppoll(fds, std::size(fds), timeout, nullptr);
        if (activity < 0)
        {
            // error in ppoll
            int error = errno;
            if (error == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from ppoll");
                continue;
            }

            LOGERROR("Failed to read from pipe - shutting down. Error: "
                     << common::safer_strerror(error) << " (" << error << ')');
            break;
        }

        if ((fds[0].revents & POLLIN) != 0)
        {
            LOGINFO("Sophos On Access Process received SIGINT - shutting down");
            sigIntMonitor->triggered();
            break;
        }

        if ((fds[1].revents & POLLIN) != 0)
        {
            LOGINFO("Sophos On Access Process received SIGTERM - shutting down");
            sigTermMonitor->triggered();
            break;
        }

        if (timeout)
        {
            m_onAccessRunner->onTimeout();
        }
    }

    // Stop the other things that can call ProcessPolicy()
    processControllerServer->tryStop();
    updateClient->tryStop();
    processControllerServerThread.reset();
    updateClientThread.reset();

    m_onAccessRunner->disableOnAccess();
}

