/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseRunner.h"

#include "Logger.h"

#include "avscanner/mountinfoimpl/Mounts.h"
#include "avscanner/mountinfoimpl/SystemPathsFactory.h"
#include "common/AbortScanException.h"
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

void BaseRunner::setMountInfo(mountinfo::IMountInfoSharedPtr ptr)
{
    m_mountInfo = std::move(ptr);
}

avscanner::mountinfo::IMountInfoSharedPtr BaseRunner::getMountInfo()
{
    if (!m_mountInfo)
    {
        auto pathsFactory = std::make_shared<mountinfoimpl::SystemPathsFactory>();
        m_mountInfo = std::make_shared<mountinfoimpl::Mounts>(pathsFactory->createSystemPaths());
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
        auto ex = e.code();
        auto errorString = "Failed to completely scan " + reportpath + " due to an error: " + e.what();
        m_scanCallbacks->scanError(errorString);
        m_returnCode = ex.value();
    }
    catch (const AbortScanException& e)
    {
        // Abort scan has already been logged in the genericFailure method
        // genericFailure -> ScanCallbackImpl::scanError(const std::string& errorMsg)
        return false;
    }
    return true;
}
