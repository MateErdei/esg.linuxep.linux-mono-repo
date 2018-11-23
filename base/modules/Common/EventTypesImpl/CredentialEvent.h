/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/EventTypes/CommonEventData.h>
#include <Common/EventTypes/ICredentialEvent.h>

#include <Common/EventTypesImpl/capnproto/Credentials.capnp.h>

namespace Common
{
    namespace EventTypesImpl
    {
    class CredentialEvent : public Common::EventTypes::ICredentialEvent
        {
        public:
            CredentialEvent() = default;

            ~CredentialEvent() = default;

            const Common::EventTypes::SessionType getSessionType() const override;
            const Common::EventTypes::EventType getEventType() const override;
            const std::string getEventTypeId() const override;
            const Common::EventTypes::UserSid getSubjectUserSid() const override;
            const Common::EventTypes::UserSid getTargetUserSid() const override;
            const unsigned long getGroupId() const override;
            const std::string getGroupName() const override;
            const unsigned long long getTimestamp() const override;
            const unsigned long getLogonId() const override;
            const Common::EventTypes::NetworkAddress getRemoteNetworkAccess() const override;
            void setSessionType(const Common::EventTypes::SessionType sessionType) override;
            void setEventType(const Common::EventTypes::EventType eventType) override;
            void setSubjectUserSid(const Common::EventTypes::UserSid subjectUserSid) override;
            void setTargetUserSid(const Common::EventTypes::UserSid targetUserSid) override;
            void setGroupId(const unsigned long groupId) override;
            void setGroupName(const std::string& groupName) override;
            void setTimestamp(const unsigned long long timestamp) override;
            void setLogonId(const unsigned long logonId) override;
            void setRemoteNetworkAccess(const Common::EventTypes::NetworkAddress remoteNetworkAccess) override;
            std::string toString() override;

            /**
            * Takes in a event object as a capn byte string.
            * @param a objectAsString
            * @returns CredentialEvent object created from the capn byte string..
            */
            CredentialEvent fromString(const std::string& objectAsString);

        private:
            Sophos::Journal::CredentialsEvent::SessionType convertToCapnSessionType(Common::EventTypes::SessionType sessionType);
            Sophos::Journal::CredentialsEvent::EventType convertToCapnEventType(Common::EventTypes::EventType eventType);
            Common::EventTypes::SessionType convertFromCapnSessionType(Sophos::Journal::CredentialsEvent::SessionType sessionType);
            Common::EventTypes::EventType convertFromCapnEventType(Sophos::Journal::CredentialsEvent::EventType eventType);
            unsigned long m_groupId;
            std::string m_groupName;
            Common::EventTypes::SessionType m_sessionType;
            Common::EventTypes::EventType m_eventType;
            Common::EventTypes::UserSid m_subjectUserSid;
            Common::EventTypes::UserSid m_targetUserSid;
            unsigned long long m_timestamp;
            unsigned long m_logonId;
            Common::EventTypes::NetworkAddress m_remoteNetworkAccess;


        };
    }

}
