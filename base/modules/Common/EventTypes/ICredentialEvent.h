/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include <string>
#include <memory>
#include <vector>

namespace Common
{
    namespace EventTypes
    {
        class ICredentialEvent
        {
        public:
            virtual const std::string getSessionType() const = 0;
            virtual const std::string getSubjectUserSID() const = 0;
            virtual const std::string getTargetUserSID() const = 0;
            virtual const std::string getTimestamp() const = 0;
            virtual const std::string getLogonID() const = 0;
            virtual const std::string getRemoteNetworkAccess() const = 0;
            virtual void setSessionType(std::string & sessionType) = 0;
            virtual void setSubjectUserSID(std::string& subjectUserSID) = 0;
            virtual void setTargetUserSID(std::string& targetUserSID) = 0;
            virtual void setTimestamp(std::string& timestamp) = 0;
            virtual void setLogonID(std::string& logonID) = 0;
            virtual void setRemoteNetworkAccess(std::string& remoteNetworkAccess) = 0;
            virtual std::string toString() = 0;
            virtual void fromString(std::string& objectAsString, ICredentialEvent* credentialEvent) = 0;

        };
    }

}
