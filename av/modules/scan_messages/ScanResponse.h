/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace scan_messages
{
    class ScanResponse
    {
    public:
        ScanResponse();

        void setThreatName(std::string);
        void setFullScanResult(std::string threatName);
        void setClean(bool);

        [[nodiscard]] std::string serialise() const;

        [[nodiscard]] bool clean() const
        {
            return m_clean;
        }

        [[nodiscard]] std::string threatName() const
        {
            return m_threatName;
        }

        [[nodiscard]] std::string fullScanResult() const
        {
            return m_fullScanResult;
        }

    private:
        bool m_clean;
        std::string m_threatName;
        std::string m_fullScanResult;
    };
}

