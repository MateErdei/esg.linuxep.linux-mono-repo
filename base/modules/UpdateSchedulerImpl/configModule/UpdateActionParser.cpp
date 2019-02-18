/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateActionParser.h"

#include <Common/UtilityImpl/StringUtils.h>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        bool isUpdateNowAction(const std::string& actionXml)
        {
            std::string strippedAction = Common::UtilityImpl::StringUtils::replaceAll(actionXml, "\n", "");
            strippedAction = Common::UtilityImpl::StringUtils::replaceAll(strippedAction, "\r", "");
            // Windows are not parsing as XML
            // https://wiki.sophos.net/display/SophosCloud/EMP%3A+sau-update-now#EMP:sau-update-now-Note
            return strippedAction == "<?xml version='1.0'?><action type=\"sophos.mgt.action.ALCForceUpdate\"/>";
        }
    } // namespace configModule
} // namespace UpdateSchedulerImpl