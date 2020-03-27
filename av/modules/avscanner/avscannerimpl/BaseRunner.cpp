/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseRunner.h"
#include "Mounts.h"

#include "common/Define.h"

#include <unixsocket/threatDetectorSocket/ScanningClientSocket.h>

using namespace avscanner::avscannerimpl;

void BaseRunner::setSocket(std::shared_ptr<unixsocket::IScanningClientSocket> ptr)
{
    m_socket = std::move(ptr);
}

std::shared_ptr<unixsocket::IScanningClientSocket> BaseRunner::getSocket()
{
    if (!m_socket)
    {
#ifdef USE_CHROOT
        const std::string unix_socket_path = "/opt/sophos-spl/plugins/av/chroot/scanning_socket";
#else
        const std::string unix_socket_path = "/opt/sophos-spl/plugins/av/var/scanning_socket";
#endif
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
        m_mountInfo = std::make_shared<Mounts>();
    }
    return m_mountInfo;
}
