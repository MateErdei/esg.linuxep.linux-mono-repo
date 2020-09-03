/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <ScanResponse.capnp.h>

namespace scan_messages
{
    class ScanResponse
    {
    public:
        ScanResponse();
        explicit ScanResponse(Sophos::ssplav::FileScanResponse::Reader reader);

        void addDetection(std::string path, std::string threatName);
        void setFullScanResult(std::string fullScanResult);
        void setErrorMsg(std::string errorMsg);

        [[nodiscard]] std::vector<std::pair<std::string, std::string>> getDetections();

        [[nodiscard]] std::string serialise() const;

        [[nodiscard]] bool allClean();

        [[nodiscard]] std::string getErrorMsg();

    private:
        std::vector<std::pair<std::string, std::string>> m_detections;
        std::string m_fullScanResult;
        std::string m_errorMsg;
    };
}

