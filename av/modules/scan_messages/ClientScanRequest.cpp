//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ClientScanRequest.h"

#include "Logger.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <ScanRequest.capnp.h>

using namespace scan_messages;

ClientScanRequest::ClientScanRequest(datatypes::ISystemCallWrapperSharedPtr sysCalls, datatypes::AutoFd& fd)
    :
    m_autoFd(std::move(fd)),
    m_syscalls(std::move(sysCalls))
{}

std::string ClientScanRequest::serialise() const
{
    ::capnp::MallocMessageBuilder message;
    Sophos::ssplav::FileScanRequest::Builder requestBuilder =
            message.initRoot<Sophos::ssplav::FileScanRequest>();

    requestBuilder.setPathname(m_path);
    requestBuilder.setScanInsideArchives(m_scanInsideArchives);
    requestBuilder.setScanInsideImages(m_scanInsideImages);
    requestBuilder.setScanType(m_scanType);
    requestBuilder.setUserID(m_userID);
    requestBuilder.setExecutablePath(m_executablePath);
    requestBuilder.setPid(m_pid);

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    return dataAsString;
}

bool ClientScanRequest::fstatIfRequired() const
{
    if (!m_autoFd.valid())
    {
        return false;
    }
    if (!m_syscalls)
    {
        return false;
    }
    if (m_fstat.st_dev == 0 && m_fstat.st_ino == 0)
    {
        int ret = m_syscalls->fstat(m_autoFd.get(), &m_fstat);
        if (ret != 0)
        {
            LOGERROR("Unable to stat: " << m_path);
            return false;
        }
    }
    return true;
}

std::optional<ClientScanRequest::hash_t> ClientScanRequest::hash() const
{
    if (!fstatIfRequired())
    {
        return {};
    }

    constexpr std::hash<unsigned long> hasher;

    const hash_t h1 = hasher(m_fstat.st_dev);
    const hash_t h2 = hasher(m_fstat.st_ino);
    return h1 ^ (h2 << 1 | h2 >> 63);
}

bool ClientScanRequest::operator==(const ClientScanRequest& other) const
{
    return (other.m_fstat.st_dev == m_fstat.st_dev &&
            other.m_fstat.st_ino == m_fstat.st_ino);
}

std::string scan_messages::getScanTypeAsStr(const E_SCAN_TYPE& scanType)
{
    switch (scanType)
    {
        case E_SCAN_TYPE_ON_ACCESS_OPEN:
        {
            return "Open";
        }
        case E_SCAN_TYPE_ON_ACCESS_CLOSE:
        {
            return "Close-Write";
        }
        case E_SCAN_TYPE_ON_DEMAND:
        {
            return "On Demand";
        }
        case E_SCAN_TYPE_SCHEDULED:
        {
            return "Scheduled";
        }
        case E_SCAN_TYPE_SAFESTORE_RESCAN:
        {
            return "SafeStore Rescan";
        }
        default:
        {
            return "Unknown";
        }
    }
}