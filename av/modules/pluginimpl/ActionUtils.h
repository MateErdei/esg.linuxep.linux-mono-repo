// Copyright 2022, Sophos Limited.  All rights reserved.

#include <Common/XmlUtilities/AttributesMap.h>

namespace pluginimpl
{
    bool isScanNowAction(const Common::XmlUtilities::AttributesMap& action);
    bool isSAVClearAction(const Common::XmlUtilities::AttributesMap& action);
    bool isCOREResetThreatHealthAction(const Common::XmlUtilities::AttributesMap& action);
    std::string getThreatID(const Common::XmlUtilities::AttributesMap& action);
}

