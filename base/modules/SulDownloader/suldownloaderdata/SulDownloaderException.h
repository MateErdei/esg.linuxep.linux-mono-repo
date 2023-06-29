/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Exceptions/IException.h"

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class SulDownloaderException : public Common::Exceptions::IException
        {
        public:
            using Common::Exceptions::IException::IException;

        };

    } // namespace suldownloaderdata
} // namespace SulDownloader
