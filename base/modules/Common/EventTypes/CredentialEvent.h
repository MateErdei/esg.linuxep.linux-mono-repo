/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CommonEventData.h"
#include "IEventType.h"
#include "PortScanningEvent.h"

namespace Common
{
    namespace EventTypes
    {
    class CredentialEvent : public Common::EventTypes::IEventType
        {
        public:

        enum SessionType
        {
            network = 0,
            networkInteractive = 1,
            interactive = 2
        };

        enum EventType
        {
            authSuccess = 0,
            authFailure = 1,
            created = 2,
            deleted = 3,
            passwordChange = 4,
            membershipChange = 5
        };
            CredentialEvent() = default;

            ~CredentialEvent() = default;

            const SessionType getSessionType() const ;
            const EventType getEventType() const ;
            const std::string getEventTypeId() const override;
            const Common::EventTypes::UserSid getSubjectUserSid() const ;
            const Common::EventTypes::UserSid getTargetUserSid() const ;
            const unsigned long getGroupId() const ;
            const std::string getGroupName() const ;
            const unsigned long long getTimestamp() const ;
            const unsigned long getLogonId() const ;
            const Common::EventTypes::NetworkAddress getRemoteNetworkAccess() const ;
            void setSessionType(const SessionType sessionType) ;
            void setEventType(const EventType eventType) ;
            void setSubjectUserSid(const Common::EventTypes::UserSid subjectUserSid) ;
            void setTargetUserSid(const Common::EventTypes::UserSid targetUserSid) ;
            void setGroupId(const unsigned long groupId) ;
            void setGroupName(const std::string& groupName) ;
            void setTimestamp(const unsigned long long timestamp) ;
            void setLogonId(const unsigned long logonId) ;
            void setRemoteNetworkAccess(const Common::EventTypes::NetworkAddress remoteNetworkAccess) ;
            const std::string toString() const override;

            /**
            * Takes in a event object as a capn byte string.
            * @param a objectAsString
            * @returns CredentialEvent object created from the capn byte string..
            */
            void fromString(const std::string& objectAsString);

        private:
            unsigned long m_groupId;
            std::string m_groupName;
            Common::EventTypes::CredentialEvent::SessionType m_sessionType;
            Common::EventTypes::CredentialEvent::EventType m_eventType;
            Common::EventTypes::UserSid m_subjectUserSid;
            Common::EventTypes::UserSid m_targetUserSid;
            unsigned long long m_timestamp;
            unsigned long m_logonId;
            Common::EventTypes::NetworkAddress m_remoteNetworkAccess;


        };

        Common::EventTypes::CredentialEvent createCredentialEvent(Common::EventTypes::UserSid sid,Common::EventTypes::CredentialEvent::EventType eventType);
    }

}
