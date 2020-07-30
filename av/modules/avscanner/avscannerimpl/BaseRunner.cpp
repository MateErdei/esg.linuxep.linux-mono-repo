/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseRunner.h"

#include "Mounts.h"
#include "SystemPathsFactory.h"

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
        const std::string unix_socket_path = pluginInstall() / "chroot/scanning_socket";
        m_socket = std::make_shared<unixsocket::ScanningClientSocket>(unix_socket_path);
    }
    return m_socket;
}

void BaseRunner::setMountInfo(std::shared_ptr<IMountInfo> ptr)
{
    m_mountInfo = std::move(ptr);
}

std::shared_ptr<IMountInfo> BaseRunner::getMountInfo()
{
    if (!m_mountInfo)
    {
        m_mountInfo = std::make_shared<Mounts>(std::make_shared<SystemPathsFactory>());
    }
    return m_mountInfo;
}
