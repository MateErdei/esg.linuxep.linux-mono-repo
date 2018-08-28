/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Credentials.h"
#include "SulDownloaderException.h"

namespace SulDownloader
{
    Credentials::Credentials(const std::string &username, const std::string &password)
            : m_username( username )
            , m_password( password)
    {
        if( !m_password.empty() && m_username.empty() )
        {
            throw SulDownloaderException( "Invalid credentials");
        }
    }

    
    const std::string &Credentials::getUsername() const
    {
        return m_username;
    }


    const std::string &Credentials::getPassword() const
    {
        return m_password;
    }

    
}
