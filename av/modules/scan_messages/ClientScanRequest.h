/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

namespace scan_messages
{

    class ClientScanRequest
    {
    public:

        ClientScanRequest() = default;

        /*
         * Sender side interface - set the path and fd, then serialise
         */
        void setPath(const std::string& path);
        std::string path();

        std::string serialise();
    private:
        std::string m_path;
    };

}

