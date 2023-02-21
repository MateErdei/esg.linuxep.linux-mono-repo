// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <zlib.h>
#include <mz_compat.h>

namespace Common::ZipUtilities
{
    class UnzipFileWrapper
    {
    public:
        explicit UnzipFileWrapper(unzFile unzipFile);
        ~UnzipFileWrapper();
        unzFile get();

    private:
        unzFile m_unzipFile;
    };
} // namespace Common::ZipUtilities
