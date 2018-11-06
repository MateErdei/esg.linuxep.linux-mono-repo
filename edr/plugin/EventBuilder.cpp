/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "EventBuilder.h"
#include "StringReplace.h"
#include "FileSystem.h"

namespace
{

    std::string getTimestamp(time_t currentTimeEpoch)
    {
        std::string timestamp = "???";

        struct tm result;
        struct tm* rp = ::gmtime_r(&currentTimeEpoch,&result);
        if (rp != nullptr)
        {
            char timebuffer[32];
            size_t timebuffersize = ::strftime(timebuffer, 32, "%Y%m%d %H%M%S", &result);
            if (timebuffersize > 0)
            {
                timestamp = timebuffer;
            }
        }
        return timestamp;
    }


    std::string buildSingleEvent(const std::string & fullFilePath, const std::string & timestamp )
    {
        static std::string eventTemplate{R"sophos(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<notification xmlns="http://www.sophos.com/EE/Event" type="sophos.mgt.msg.event.threat"
    description="@@DESCRIPTION@@"
    timestamp="@@TIMESTAMP@@">
   <threat
        type="1"
        name="VirusFile"
        scanType="201"
        status="200"
        id="T@@ID@@"
        idSource="Tmd5(dis,vName,path)">
       <user userId="TestUser" domain="" />
       <item file="@@FILE@@" path="@@PATH@@/"/>
       <action action="116"/>
   </threat>
</notification>
)sophos"};

        FileSystem fileSystem;
        std::string fileName = fileSystem.baseName(fullFilePath);
        std::string directory = fileSystem.dirPath(fullFilePath);


        std::string description = "Virus File found in " + fullFilePath;



        std::size_t eventHash = std::hash<std::string>{}(fullFilePath);
        std::string id = std::to_string(eventHash);

        Example::KeyValueCollection keyvalues{
                {"@@DESCRIPTION@@", description},
                {"@@TIMESTAMP@@", timestamp},
                {"@@ID@@", id},
                {"@@FILE@@", fileName},
                {"@@PATH@@", directory}
        };

        return Example::orderedStringReplace(eventTemplate, keyvalues);
    }
}


namespace Example
{
    std::vector<std::string> buildEvents(const ScanReport &scanReport)
    {
        std::vector<std::string> events;
        std::string timestamp = getTimestamp(scanReport.getFinishTime());
        for( auto & infectedFile : scanReport.getInfections())
        {
            events.emplace_back( buildSingleEvent(infectedFile, timestamp) );
        }
        return events;
    }
}
