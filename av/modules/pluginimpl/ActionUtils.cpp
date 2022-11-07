// Copyright 2022, Sophos Limited.  All rights reserved.

#include "ActionUtils.h"

namespace pluginimpl
{
    bool isScanNowAction(const Common::XmlUtilities::AttributesMap& action)
    {
        if (action.lookup("a:action").value("type", "") == "ScanNow")
        {
            return true;
        }
        return false;
    }

    bool isSAVClearAction(const Common::XmlUtilities::AttributesMap& action)
    {
        if (action.lookup("action").value("type", "") == "sophos.mgt.action.SAVClearFromList")
        {
            return true;
        }
        return false;
    }

    std::string getThreatID(const Common::XmlUtilities::AttributesMap& action)
    {
        auto threats =  action.entitiesThatContainPath("action/threat-set/threat");
        for (const auto& threat : threats)
        {
            auto threatDetails = action.lookup(threat);
            std::string threatName = threatDetails.value("id");
            if (threatName.empty())
            {
                continue;
            }
            return threatName;
        }
        throw std::runtime_error("No ID found for threat in Sav clear action");
    }
}