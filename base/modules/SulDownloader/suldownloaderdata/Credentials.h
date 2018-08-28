/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>

namespace SulDownloader
{

    namespace suldownloaderdata
    {
        class Credentials
        {

        public:
            explicit Credentials(const std::string& username = "", const std::string& password = "");

            const std::string& getUsername() const;

            const std::string& getPassword() const;

            bool operator==(const Credentials& rhs) const
            {
                return (m_username == rhs.m_username && m_password == rhs.m_password);
            }

            bool operator!=(const Credentials& rhs) const
            {
                return !operator==(rhs);
            }

        private:
            std::string m_username;
            std::string m_password;
        };
    }

}

