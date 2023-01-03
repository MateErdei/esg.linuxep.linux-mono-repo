// Copyright 2023 Sophos Limited. All rights reserved.

#include "StatusFile.h"

#include <tuple>
#include <utility>

using namespace common;

StatusFile::StatusFile(std::string path)
    : m_path(std::move(path))
{
}

void StatusFile::enabled()
{
}

void StatusFile::disabled()
{
}

bool StatusFile::isEnabled(const std::string& path)
{
    std::ignore = path;
    return false;
}
