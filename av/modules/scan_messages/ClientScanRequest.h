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
         * Sender side interface - set the fields, then serialise
         */
        void setPath(const std::string& path);

        void setScanInsideArchives(bool a)
        {
            m_scanInsideArchives = a;
        }

        [[nodiscard]] std::string serialise() const;

    protected:
        std::string m_path;
        bool m_scanInsideArchives = false;
    };

}

