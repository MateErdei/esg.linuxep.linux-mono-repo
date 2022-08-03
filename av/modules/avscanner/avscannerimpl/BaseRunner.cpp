// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "BaseRunner.h"

#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"
#include "common/AbortScanException.h"
#include "common/ErrorCodes.h"
#include "common/ScanManuallyInterruptedException.h"
#include "common/ScanInterruptedException.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>


using namespace avscanner::avscannerimpl;

namespace fs = sophos_filesystem;

static fs::path pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    try
    {
        return appConfig.getData("PLUGIN_INSTALL");
    }
    catch (const std::out_of_range&)
    {
        return "/opt/sophos-spl/plugins/av";
    }
}

BaseRunner::BaseRunner()
{
    // Ensure signal handlers are created before logging is set up
    std::ignore = common::SigIntMonitor::getSigIntMonitor();
    std::ignore = common::SigTermMonitor::getSigTermMonitor(false);
    std::ignore = common::SigHupMonitor::getSigHupMonitor();
}

void BaseRunner::setSocket(std::shared_ptr<unixsocket::IScanningClientSocket> ptr)
{
    m_socket = std::move(ptr);
}

std::shared_ptr<unixsocket::IScanningClientSocket> BaseRunner::getSocket()
{
    if (!m_socket)
    {
        const std::string unix_socket_path = pluginInstall() / "chroot/var/scanning_socket";
        m_socket = std::make_shared<unixsocket::ScanningClientSocket>(unix_socket_path);
    }
    return m_socket;
}

void BaseRunner::setMountInfo(mount_monitor::mountinfo::IMountInfoSharedPtr ptr)
{
    m_mountInfo = std::move(ptr);
}

mount_monitor::mountinfo::IMountInfoSharedPtr BaseRunner::getMountInfo()
{
    if (!m_mountInfo)
    {
        auto pathsFactory = std::make_shared<mount_monitor::mountinfoimpl::SystemPathsFactory>();
        m_mountInfo = std::make_shared<mount_monitor::mountinfoimpl::Mounts>(pathsFactory->createSystemPaths());
    }
    return m_mountInfo;
}

bool BaseRunner::walk(filewalker::FileWalker& filewalker,
                      const sophos_filesystem::path& abspath,
                      const std::string& reportpath)
{
    try
    {
        filewalker.walk(abspath);
    }
    catch (fs::filesystem_error& e)
    {
        auto errorString = "Failed to completely scan " + reportpath + " due to an error: " + e.what();
        m_scanCallbacks->scanError(errorString, e.code());
        m_returnCode = e.code().value();
    }
    catch (const ScanManuallyInterruptedException& e)
    {
        m_returnCode = common::E_EXECUTION_MANUALLY_INTERRUPTED;
        return false;
    }
    catch (const ScanInterruptedException& e)
    {
        m_returnCode = common::E_EXECUTION_INTERRUPTED;
        return false;
    }
    catch (const common::AbortScanException& e)
    {
        // Abort scan has already been logged in the genericFailure method
        // genericFailure -> ScanCallbackImpl::scanError(const std::string& errorMsg)
        m_returnCode = common::E_SCAN_ABORTED;
        return false;
    }
    return true;
}
