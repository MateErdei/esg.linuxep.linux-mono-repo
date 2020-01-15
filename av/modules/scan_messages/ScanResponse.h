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

        std::string serialise();

        void setClean(bool);
        bool clean() { return m_clean;}

    private:
        bool m_clean;
    };
}

