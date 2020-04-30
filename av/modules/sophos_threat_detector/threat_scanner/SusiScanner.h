/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IThreatScanner.h"
#include "SusiWrapper.h"
#include "ISusiWrapperFactory.h"

#include <scan_messages/ScanResponse.h>
#include <datatypes/AutoFd.h>


namespace threat_scanner
{
    class SusiScanner : public IThreatScanner
    {
    public:
        explicit SusiScanner(const std::shared_ptr<ISusiWrapperFactory>& susiWrapperFactory, bool scanArchives);

        scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const std::string& file_path) override;


    private:
        std::shared_ptr<ISusiWrapper> m_susi;
    };
}