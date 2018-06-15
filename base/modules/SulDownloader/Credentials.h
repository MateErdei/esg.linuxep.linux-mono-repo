/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_CREDENTIALS_H
#define EVEREST_CREDENTIALS_H

#include <string>

namespace SulDownloader
{

    class Credentials
    {

    public:
        explicit Credentials( const std::string & username = "", const std::string & password = "" );
        const std::string & getUsername() const;

        const std::string & getPassword() const;
    private:
        std::string m_username;
        std::string m_password;
    };

}
#endif //EVEREST_CREDENTIALS_H
