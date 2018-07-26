/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Exceptions/IException.h"

namespace Common
{
    namespace FileSystem
    {
        class IFileSystemException : public Common::Exceptions::IException
        {
        public:
            explicit IFileSystemException(const std::string& what)
                    : Common::Exceptions::IException(what)
            {}
        };
    }
}


