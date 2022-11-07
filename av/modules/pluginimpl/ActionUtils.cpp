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
        return action.lookup("action/threat-set/threat").value("id", "");
    }
}