// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessScanRequest.h"

#include <stdexcept>

using sophos_on_access_process::onaccessimpl::OnAccessScanRequest;

OnAccessScanRequest::unique_t OnAccessScanRequest::uniqueMarker() const
{
    if (!fstatIfRequired())
    {
        throw std::runtime_error("Unable to create unique value - fstat failed");
    }

    return unique_t{m_fstat.st_dev, m_fstat.st_ino};
}
