// Copyright 2023 Sophos Limited. All rights reserved.
#include "ActionProcessor.h"

#include "Logger.h"

#include "Common/XmlUtilities/AttributesMap.h"

std::optional<bool> Plugin::ActionProcessor::processIsolateAction(const std::string& xml)
{
    if (xml.empty())
    {
        LOGERROR("Received empty action!");
        return std::nullopt;
    }
    if (xml[0] != '<')
    {
        // Might be JSON - don't produce error
        LOGDEBUG("Ignoring non-XML action: " << xml);
        return std::nullopt;
    }

    try
    {
        auto attributeMap = Common::XmlUtilities::parseXml(xml);
        auto action = attributeMap.lookup("action");
        if (action.value("type") != "sophos.mgt.action.IsolationRequest")
        {
            LOGERROR("Incorrect action type: " << action.value("type"));
            return std::nullopt;
        }
        auto enabledList = attributeMap.lookupMultiple("action/enabled");
        if (enabledList.empty())
        {
            LOGERROR("No action or enabled elements");
            return std::nullopt;
        }
        if (enabledList.size() > 1)
        {
            LOGERROR("Multiple enabled elements!");
            return std::nullopt;
        }
        auto contents = enabledList[0].contents();
        if (contents == "true")
        {
            return true;
        }
        else if (contents == "false")
        {
            return false;
        }
        else
        {
            LOGERROR("Contents of enabled element are invalid: " << contents);
        }
    }
    catch (const Common::XmlUtilities::XmlUtilitiesException& ex)
    {
        LOGDEBUG("Failed to parse action: " << xml);
        LOGERROR("Failed to parse action: " << ex.what());
    }

    return std::nullopt;
}
