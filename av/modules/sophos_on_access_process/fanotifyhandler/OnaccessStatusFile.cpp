// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnaccessStatusFile.h"

#include "common/PluginUtils.h"

#include <fstream>

using namespace sophos_on_access_process::fanotifyhandler;

OnaccessStatusFile::OnaccessStatusFile()
: m_statusFilePath(common::getPluginInstallPath() / "var/onaccess.status")
{
    writeStatusFile();
}

void OnaccessStatusFile::setStatus(sophos_on_access_process::fanotifyhandler::OnaccessStatus status)
{
    std::unique_lock<std::mutex> lock(m_lock);
    m_status = status;
    writeStatusFile();
}

void OnaccessStatusFile::writeStatusFile()
{
    std::ofstream statusFile(m_statusFilePath.c_str());
    statusFile << m_status;
}