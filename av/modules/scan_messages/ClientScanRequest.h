/******************************************************************************************************

Copyright 2020-2021
******************************************************************************************************/

#pragma once

#include "ScanType.h"

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

        void setScanInsideImages(bool a)
        {

            m_scanInsideImages = a;
        }

        void setScanType(E_SCAN_TYPE scanType);

        void setUserID(const std::string& userID);

        [[nodiscard]] std::string serialise() const;
        [[nodiscard]] std::string getPath() const { return m_path; };

    protected:
        std::string m_path;
        bool m_scanInsideArchives = false;
        bool m_scanInsideImages = false;
        std::string m_userID;
        E_SCAN_TYPE m_scanType = E_SCAN_TYPE_UNKNOWN;
    };

}

