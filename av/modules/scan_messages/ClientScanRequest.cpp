// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ClientScanRequest.h"

#include "Logger.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include "scan_messages/ScanRequest.capnp.h"

using namespace scan_messages;

ClientScanRequest::ClientScanRequest(Common::SystemCallWrapper::ISystemCallWrapperSharedPtr sysCalls, datatypes::AutoFd& fd)
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
    requestBuilder.setDetectPUAs(m_detectPUAs);
    requestBuilder.setScanType(m_scanType);
    requestBuilder.setUserID(m_userID);
    requestBuilder.setExecutablePath(m_executablePath);
    requestBuilder.setPid(m_pid);

    {
        auto exclusions = requestBuilder.initExcludePUAs(excludedPUAs_.size());
        for (unsigned i = 0; i < excludedPUAs_.size(); i++)
        {
            exclusions.set(i, excludedPUAs_[i]);
        }
    }

    kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
    kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
    std::string dataAsString(bytes.begin(), bytes.end());

    return dataAsString;
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