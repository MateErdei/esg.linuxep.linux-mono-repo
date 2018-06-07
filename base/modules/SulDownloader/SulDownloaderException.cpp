//
// Created by pair on 06/06/18.
//

#include "SulDownloaderException.h"
namespace SulDownloader
{
    SulDownloaderException::SulDownloaderException(std::string message)
    : m_message( message)
    {

    }

    const char *SulDownloaderException::what() const noexcept
    {
        return m_message.c_str();
    }
}