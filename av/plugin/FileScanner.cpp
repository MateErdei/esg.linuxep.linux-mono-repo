/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <algorithm>
#include "FileScanner.h"
#include "Logger.h"

namespace Example
{

    void FileScanner::setExclusions(std::vector<std::string> exclusions)
    {
        m_exclusions = exclusions;
    }

    ScanReport FileScanner::scan()
    {
        ScanReport scanReport;
        FileSystem fileSystem;
        char * envpath = secure_getenv("PATHTOSCAN");
        std::string scanpath = envpath != nullptr? envpath : "/tmp";
        LOGSUPPORT("Scan path: " << scanpath);
        scanPathRecursively(scanpath, scanReport, fileSystem);

        scanReport.scanFinished();

        return scanReport;
    }

    void FileScanner::scanPathRecursively(const std::string &filePath, ScanReport &scanReport,
                                          const FileSystem &fileSystem)
    {
        //LOGSUPPORT("Scanning " << filePath);
        {
            auto excluded = std::find(m_exclusions.begin(), m_exclusions.end(), filePath );
            if( excluded != m_exclusions.end())
            {
                LOGSUPPORT("Skipping excluded path from scanner: " << *excluded);
                return;
            }
        }

        try
        {
            if( fileSystem.isDirectory(filePath))
            {
                for( auto & childPath : fileSystem.listFileSystemEntries(filePath))
                {
                    scanPathRecursively(childPath, scanReport, fileSystem);
                }
            }
            else if ( fileSystem.isRegularFile(filePath))
            {
                scanFile(filePath, scanReport, fileSystem);
            }
            else
            {
                LOGSUPPORT( "Ignoring entry for scanner: " << filePath);
            }
        }
        catch (std::exception & ex)
        {
            LOGERROR(ex.what());
        }

    }

    void FileScanner::scanFile(const std::string &filePath, ScanReport &scanReport, const FileSystem &fileSystem)
    {
        //LOGDEBUG("Scan file: " << filePath);
        constexpr double MB = 1024*1024;
        std::string content = fileSystem.readFile(filePath);

        scanReport.reportNewFileScanned( double(content.size())/MB);

        if( content.find("VIRUS") != std::string::npos)
        {
            LOGSUPPORT("Threat detected: " << filePath);
            scanReport.reportInfection(filePath);
        }
    }

}
