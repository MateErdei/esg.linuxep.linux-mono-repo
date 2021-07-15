/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <ScanResponse.capnp.h>

namespace scan_messages
{
    struct DetectionContainer
    {
        std::string path;
        std::string name;
        std::string sha256;
    };

    class ScanResponse
    {
    public:
        ScanResponse();
        explicit ScanResponse(Sophos::ssplav::FileScanResponse::Reader reader);

        void addDetection(std::string path, std::string threatName, std::string sha256);
        void setFullScanResult(std::string fullScanResult);
        void setErrorMsg(std::string errorMsg);

        [[nodiscard]] std::vector<DetectionContainer> getDetections();

        [[nodiscard]] std::string serialise() const;

        [[nodiscard]] bool allClean();

        [[nodiscard]] std::string getErrorMsg();

    private:
        std::vector<DetectionContainer> m_detections;
        std::string m_fullScanResult;
        std::string m_errorMsg;
    };
}

