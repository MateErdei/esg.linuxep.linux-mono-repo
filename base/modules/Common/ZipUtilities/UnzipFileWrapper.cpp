// Copyright 2023 Sophos Limited. All rights reserved.

#include "UnzipFileWrapper.h"

Common::ZipUtilities::UnzipFileWrapper::UnzipFileWrapper(unzFile unzipFile) : m_unzipFile(unzipFile) {}

Common::ZipUtilities::UnzipFileWrapper::~UnzipFileWrapper()
{
    unzClose(m_unzipFile);
}

unzFile Common::ZipUtilities::UnzipFileWrapper::get()
{
    return m_unzipFile;
}