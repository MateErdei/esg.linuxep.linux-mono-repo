// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IFileSystemException.h"

namespace Common::FileSystem
{
    class IFileTooLargeException : public IFileSystemException
    {
    public:
        using IFileSystemException::IFileSystemException;
    };
}
