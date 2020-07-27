/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Logger.h"
#include "MonitorDir.h"
#include <Common/FileSystem/IFileSystem.h>
namespace CommsComponent {
    MonitorDir::MonitorDirListener::MonitorDirListener(const std::string& directoryPath, const std::string& fileNameContainsFilter):
        m_directoryPath(directoryPath), m_fileNameContainsFilter(fileNameContainsFilter)
    {

    }

    MonitorDir::MonitorDirListener::~MonitorDirListener()
    {        
    }

    std::string MonitorDir::MonitorDirListener::getPath() const
    {
        return m_directoryPath; 
    }

    void MonitorDir::MonitorDirListener::fileMoved(const std::string& filename)
    {
        // apply filter: either filter is empty, or the filename must contain the filter string
        if ( m_fileNameContainsFilter.empty() || filename.find(m_fileNameContainsFilter) != std::string::npos)
        {
            LOGINFO("Detected new file to process: " << filename); 
            m_channel.push( Common::FileSystem::join(m_directoryPath,filename)); 
        }
        else
        {
            LOGSUPPORT("File movement detected but ignored as does not pass the required filter: " << filename); 
        }        
    }

    void MonitorDir::MonitorDirListener::watcherActive(bool /*active*/)
    {

    }

    MessageChannel& MonitorDir::MonitorDirListener::channel()
    {
        return m_channel; 
    }


    MonitorDir::MonitorDir(const std::string & directoryPath, const std::string & fileNameContainsFilter): 
      m_monitorDirListener{directoryPath, fileNameContainsFilter},
      m_directoryWatcher(Common::DirectoryWatcher::createDirectoryWatcher())
    {
        m_directoryWatcher->addListener(m_monitorDirListener); 
        m_directoryWatcher->startWatch(); 
    }

    MonitorDir::~MonitorDir()
    {
    }

    /* may throw MonitorDirClosedException after stop is issued*/
    std::optional<std::string> MonitorDir::next(std::chrono::milliseconds timeout)
    {
        std::string message; 
        try
        {
            if ( m_monitorDirListener.channel().pop(message, timeout))
            {
                return message; 
            }
            return std::nullopt; 
        }catch( ChannelClosedException & )
        {
            throw MonitorDirClosedException("closed"); 
        }
    }

    void MonitorDir::stop()
    {
        m_monitorDirListener.channel().pushStop();         
    }

} // namespace CommsComponent