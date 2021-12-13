/******************************************************************************************************

Copyright 2020-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <ScanResponse.capnp.h>

namespace scan_messages
{
    struct Detection
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

        void addDetection(const std::string& filePath, const std::string& threatName, const std::string& sha256);
        void setErrorMsg(std::string errorMsg);

        [[nodiscard]] std::vector<Detection> getDetections();

        [[nodiscard]] std::string serialise() const;

        [[nodiscard]] bool allClean();

        [[nodiscard]] std::string getErrorMsg();

    private:
        std::vector<Detection> m_detections;
        std::string m_errorMsg;
    };
}

