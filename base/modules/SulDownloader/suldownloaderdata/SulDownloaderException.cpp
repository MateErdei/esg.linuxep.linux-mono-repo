/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloaderException.h"
namespace SulDownloader
{
    SulDownloaderException::SulDownloaderException(std::string message)
    : m_message( std::move(message) )
    {

    }

    const char *SulDownloaderException::what() const noexcept
    {
        return m_message.c_str();
    }
}