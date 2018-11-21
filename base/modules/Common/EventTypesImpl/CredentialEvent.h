/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/EventTypes/CommonEventData.h"
#include <Common/EventTypes/ICredentialEvent.h>

#include <Common/EventTypesImpl/capnproto/generated/Credentials.capnp.h>

namespace Common
{
    namespace EventTypesImpl
    {
    class CredentialEvent : public Common::EventTypes::ICredentialEvent
        {
        public:
            CredentialEvent();
            //CredentialEvent(std::string& sessionType, std::string& subjectUserSID, std::string& targetUserSID, std::string& timestamp, std::string& logonID, std::string& remoteNetworkAccess);
            ~CredentialEvent() = default;

            const Common::EventTypes::SessionType getSessionType() const override;
            const Common::EventTypes::EventType getEventType() const override;
            const std::string getEventTypeId() const override;
            const Common::EventTypes::UserSID getSubjectUserSID() const override;
            const Common::EventTypes::UserSID getTargetUserSID() const override;
            const unsigned long getGroupId() const override;
            const std::string getGroupName() const override;
            const unsigned long long getTimestamp() const override;
            const unsigned long getLogonID() const override;
            const Common::EventTypes::NetworkAddress getRemoteNetworkAccess() const override;
            void setSessionType(Common::EventTypes::SessionType sessionType) override;
            void setEventType(Common::EventTypes::EventType eventType) override;
            void setSubjectUserSID(Common::EventTypes::UserSID subjectUserSID) override;
            void setTargetUserSID(Common::EventTypes::UserSID targetUserSID) override;
            void setGroupId(unsigned long groupId) override;
            void setGroupName(std::string& groupName) override;
            void setTimestamp(unsigned long long timestamp) override;
            void setLogonID(unsigned long logonID) override;
            void setRemoteNetworkAccess(Common::EventTypes::NetworkAddress remoteNetworkAccess) override;
            std::string toString() override;
            ICredentialEvent* fromString(std::string& objectAsString) override;

        private:
            Sophos::Journal::CredentialsEvent::SessionType convertToCapnSessionType(Common::EventTypes::SessionType sessionType);
            Sophos::Journal::CredentialsEvent::EventType convertToCapnEventType(Common::EventTypes::EventType eventType);
            Common::EventTypes::SessionType convertFromCapnSessionType(Sophos::Journal::CredentialsEvent::SessionType sessionType);
            Common::EventTypes::EventType convertFromCapnEventType(Sophos::Journal::CredentialsEvent::EventType eventType);
            unsigned long m_groupID;
            std::string m_groupName;
            Common::EventTypes::SessionType m_sessionType;
            Common::EventTypes::EventType m_eventType;
            Common::EventTypes::UserSID m_subjectUserID;
            Common::EventTypes::UserSID m_targetUserSID;
            unsigned long long m_timestamp;
            unsigned long m_logonID;
            Common::EventTypes::NetworkAddress m_remoteNetworkAccess;


        };
    }

}
