/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <exception>
#include <string>
namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class SulDownloaderException
                : public std::exception
        {
        public:
            explicit SulDownloaderException(std::string message);

            const char* what() const noexcept override;

        private:
            std::string m_message;
        };

    }
}

