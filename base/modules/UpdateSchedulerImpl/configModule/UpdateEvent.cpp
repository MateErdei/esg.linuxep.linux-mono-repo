/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/UtilityImpl/StringUtils.h>
#include "UpdateEvent.h"

namespace
{
    using UpdateEvent = UpdateSchedulerImpl::configModule::UpdateEvent;
std::string insertTemplate{
"        <insert>@@entryname@@</insert>"
};

std::string messageTemplate{
R"sophos(
    <message>
      <message_inserts>
@@inserts@@
      </message_inserts>
    </message>)sophos"};

std::string eventTemplate{R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>@@timestamp@@</timestamp>
  <appInfo>
    <number>@@messagenumber@@</number>@@message@@
  <updateSource>@@updateSource@@</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos"};

    std::string getMessageContent(const UpdateEvent& event)
{
    if ( event.Messages.empty())
    {
        return std::string();
    }
    std::string allInserts;
    for( auto & e : event.Messages)
    {
        std::string insertEntry;
        if ( !e.PackageName.empty())
        {
            insertEntry += Common::UtilityImpl::StringUtils::orderedStringReplace(insertTemplate, {{"@@entryname@@", e.PackageName}});
        }
        if( !e.ErrorDetails.empty())
        {
            if( !insertEntry.empty())
            {
                insertEntry += "\n";
            }
            //FIXME LINUXEP-6473: Get the correct error code to send to Central.
            insertEntry += Common::UtilityImpl::StringUtils::orderedStringReplace(insertTemplate, {{"@@entryname@@", "CodeErrorA"}});
        }

        if( !allInserts.empty())
        {
            allInserts += "\n";
        }
        allInserts += insertEntry;
    }

    return Common::UtilityImpl::StringUtils::orderedStringReplace(messageTemplate, {{"@@inserts@@", allInserts}});
}


std::string eventXML( const std::string & timestamp, const std::string & messageNumber, const std::string & messageElement, const std::string & updateSource)
{
    return Common::UtilityImpl::StringUtils::orderedStringReplace(eventTemplate, {
            {"@@timestamp@@", timestamp},
            {"@@messagenumber@@", messageNumber},
            {"@@message@@", messageElement},
            {"@@updateSource@@", updateSource}
    });
}
}


namespace UpdateSchedulerImpl
{
    namespace configModule
    {

        using namespace UpdateScheduler;

        std::string serializeUpdateEvent(const UpdateEvent& updateEvent, const IMapHostCacheId& iMapHostCacheId,
                                         const Common::UtilityImpl::IFormattedTime& iFormattedTime)
        {
            std::string timestamp = iFormattedTime.currentTime();
            std::string updateSource = iMapHostCacheId.cacheID(updateEvent.UpdateSource);
            std::string messageContent = getMessageContent(updateEvent);
            std::string messageNumber = std::to_string(updateEvent.MessageNumber);

            return eventXML(timestamp, messageNumber, messageContent, updateSource);
        }
    }
}