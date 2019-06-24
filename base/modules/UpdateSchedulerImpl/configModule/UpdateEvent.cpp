/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateEvent.h"

#include "PropertyTreeHelper.h"
#include <set>

namespace
{
    using UpdateEvent = UpdateSchedulerImpl::configModule::UpdateEvent;
    namespace pt = boost::property_tree;

    /**
     * Insert message node for non-empty messages
     * <message>
     *  <message_inserts>
     *   <insert>Value</insert>
     *  </message_inserts>
     * </message>
     *
     * TODO: LINUXEP-6473: Get the correct error code to send to Central.
     * We'll need to work out precisely what to include based on the code.
     *
     * @param event
     * @param addInfoNode
     */
    void insertMessageContent(const UpdateEvent& event, pt::ptree& addInfoNode)
    {
        using EventMessageNumber = UpdateSchedulerImpl::configModule::EventMessageNumber;

        if (event.Messages.empty())
        {
            return;
        }

        pt::ptree::assoc_iterator it = addInfoNode.find("updateSource"); // insert adds node before iterator element
        assert(it != addInfoNode.not_found());
        if (it == addInfoNode.not_found())
        {
            return;
        }

        auto sendName = [&](auto e, auto messageInsertsNode)
        {
            if (!e.PackageName.empty())
            {
                messageInsertsNode->add("insert", e.PackageName);
            }
        };

        auto sendDetails = [&](auto e, auto messageInsertsNode)
        {
            if (!e.ErrorDetails.empty())
            {
                messageInsertsNode->add("insert", e.ErrorDetails);
            }
        };

        std::set<int> errorCodes = {EventMessageNumber::INSTALLFAILED,EventMessageNumber::INSTALLCAUGHTERROR,
                                    EventMessageNumber::DOWNLOADFAILED,EventMessageNumber::SINGLEPACKAGEMISSING,
                                    EventMessageNumber::CONNECTIONERROR};

        if (errorCodes.find(event.MessageNumber) != errorCodes.end())
        {
            addInfoNode.insert(addInfoNode.to_iterator(it), { "message", pt::ptree() });
            auto& messageNode = addInfoNode.get_child("message");
            auto& messageInsertsNode = messageNode.put("message_inserts", "");

            auto e = event.Messages[0]; // Sophos Central only supports sending one message

            switch(event.MessageNumber)
            {
                case(EventMessageNumber::INSTALLFAILED):
                    sendName(e, &messageInsertsNode);
                    if (!e.PackageName.empty()) // Only send error details if package name is sent first
                    {
                        sendDetails(e, &messageInsertsNode);
                    }
                    break;
                case(EventMessageNumber::INSTALLCAUGHTERROR):
                    sendDetails(e, &messageInsertsNode);
                    break;
                case(EventMessageNumber::DOWNLOADFAILED):
                    sendName(e, &messageInsertsNode);
                    if (!e.PackageName.empty()) // Only send error details if package name is sent first
                    {
                        sendDetails(e, &messageInsertsNode);
                    }
                    break;
                case(EventMessageNumber::SINGLEPACKAGEMISSING):
                    sendName(e, &messageInsertsNode);
                    break;
                case(EventMessageNumber::CONNECTIONERROR):
                    sendDetails(e, &messageInsertsNode);
                    break;
                default:
                    break;
            }
        }
    }


    std::string eventXML(
        const UpdateEvent& updateEvent,
        const std::string& timestamp,
        const std::string& messageNumber,
        const std::string& updateSource)
    {
        static const std::string L_EVENT_TEMPLATE{ R"sophos(<?xml version="1.0"?>
<event xmlns="http://www.sophos.com/EE/AUEvent" type="sophos.mgt.entityAppEvent">
  <timestamp>@@timestamp@@</timestamp>
  <appInfo>
    <number>@@messagenumber@@</number><!-- @@message@@ -->
    <updateSource>@@updateSource@@</updateSource>
  </appInfo>
  <entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo>
</event>)sophos" };

        auto tree = UpdateSchedulerImpl::configModule::parseString(L_EVENT_TEMPLATE);
        auto& appInfoNode = tree.get_child("event.appInfo");

        insertMessageContent(updateEvent, appInfoNode);

        tree.put("event.timestamp", timestamp);
        tree.put("event.appInfo.number", messageNumber);
        tree.put("event.appInfo.updateSource", updateSource);

        return UpdateSchedulerImpl::configModule::toString(tree);
    }
} // namespace

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        using namespace UpdateScheduler;

        std::string serializeUpdateEvent(
            const UpdateEvent& updateEvent,
            const IMapHostCacheId& iMapHostCacheId,
            const Common::UtilityImpl::IFormattedTime& iFormattedTime)
        {
            std::string timestamp = iFormattedTime.currentTime();
            std::string updateSource = iMapHostCacheId.cacheID(updateEvent.UpdateSource);
            if (updateSource.empty())
            {
                // https://wiki.sophos.net/display/SophosCloud/EMP%3A+event-alc
                // setting to Sophos as stated in the wiki page above
                updateSource = "Sophos";
            }
            std::string messageNumber = std::to_string(updateEvent.MessageNumber);

            return eventXML(updateEvent, timestamp, messageNumber, updateSource);
        }
    } // namespace configModule
} // namespace UpdateSchedulerImpl