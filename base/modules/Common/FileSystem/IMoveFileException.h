// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IFileSystemException.h"

namespace Common::FileSystem
{
    class IMoveFileException : public IFileSystemException
    {
    public:
        using IFileSystemException::IFileSystemException;
    };
}
