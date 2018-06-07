//
// Created by pair on 05/06/18.
//

#ifndef EVEREST_PROXY_H
#define EVEREST_PROXY_H

#include <string>

#include "Credentials.h"

namespace SulDownloader
{
    class Proxy
    {
    public:

        explicit Proxy( const std::string & url = "", const Credentials & credentials = Credentials()  );

        const Credentials & getCredentials() const;

        const std::string &getUrl() const;
        bool empty() const;

    private:

        std::string m_url;
        Credentials m_credentials;
    };
}

#endif //EVEREST_PROXY_H
