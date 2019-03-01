/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateEvent.h"

#include <Common/UtilityImpl/StringUtils.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace
{
    using UpdateEvent = UpdateSchedulerImpl::configModule::UpdateEvent;
    namespace pt = boost::property_tree;

    void insertMessageContent(const UpdateEvent& event, pt::ptree& addInfoNode)
    {
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

        addInfoNode.insert(addInfoNode.to_iterator(it), { "message", pt::ptree() });
        auto& messageNode = addInfoNode.get_child("message");
        auto& messageInsertsNode = messageNode.put("message_inserts", "");

        for (auto& e : event.Messages)
        {
            if (!e.PackageName.empty())
            {
                messageInsertsNode.add("insert", e.PackageName);
            }
            /*if( !e.ErrorDetails.empty())
            {
                //FIXME LINUXEP-6473: Get the correct error code to send to Central.
                addInfoNode.add("message.message_inserts.insert","CodeErrorA");
            }*/
        }
    }

    boost::property_tree::ptree parseString(const std::string& input)
    {
        namespace pt = boost::property_tree;
        std::istringstream i(input);
        pt::ptree tree;
        pt::xml_parser::read_xml(i, tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments);
        return tree;
    }

    std::string toString(const boost::property_tree::ptree& tree)
    {
        namespace pt = boost::property_tree;
        std::ostringstream ost;
        pt::xml_parser::write_xml(ost, tree);
        return ost.str();
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

        auto tree = parseString(L_EVENT_TEMPLATE);
        auto& appInfoNode = tree.get_child("event.appInfo");
        insertMessageContent(updateEvent, appInfoNode);

        tree.put("event.timestamp", timestamp);
        tree.put("event.appInfo.number", messageNumber);
        tree.put("event.appInfo.updateSource", updateSource);

        return toString(tree);
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
            std::string messageNumber = std::to_string(updateEvent.MessageNumber);

            return eventXML(updateEvent, timestamp, messageNumber, updateSource);
        }
    } // namespace configModule
} // namespace UpdateSchedulerImpl