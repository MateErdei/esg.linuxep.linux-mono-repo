/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "Common/DirectoryWatcher/IDirectoryWatcher.h"

class DirectoryWatcherListener: public Common::DirectoryWatcher::IDirectoryWatcherListener
{
public:
    explicit DirectoryWatcherListener(const std::string &path)
            : m_Path(path), m_File(""), m_Active(false), m_HasData(false)
    {}

    std::string getPath() const override
    {
        return m_Path;
    }

    void fileMoved(const std::string & filename) override
    {
        std::lock_guard<std::mutex> guard(m_FilenameMutex);
        m_File = filename;
        m_HasData = true;
    }

    void watcherActive(bool active) override
    {
        m_Active = active;
    }

    bool hasData()
    {
        return m_HasData;
    }

    std::string popFile()
    {
        std::lock_guard<std::mutex> guard(m_FilenameMutex);
        std::string data = m_File;
        m_File.clear();
        m_HasData = false;
        return data;
    }

public:
    std::string m_Path;
    std::string m_File;
    bool m_Active;
    bool m_HasData;
    std::mutex m_FilenameMutex;
};


