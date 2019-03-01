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
        const unsigned long G_UNSET_ID = 0xFFFFFFFF;

        class CredentialEvent : public virtual Common::EventTypes::IEventType
        {
        public:
            using windows_timestamp_t = Common::EventTypes::windows_timestamp_t;
            using login_id_t = unsigned long;

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

            CredentialEvent();

            ~CredentialEvent() = default;

            /*
             * Getters
             */
            SessionType getSessionType() const;
            EventType getEventType() const;
            std::string getEventTypeId() const override;
            Common::EventTypes::UserSid getSubjectUserSid() const;
            Common::EventTypes::UserSid getTargetUserSid() const;
            unsigned long getGroupId() const;
            std::string getGroupName() const;
            windows_timestamp_t getTimestamp() const;
            login_id_t getLogonId() const;
            Common::EventTypes::NetworkAddress getRemoteNetworkAccess() const;
            Common::EventTypes::SophosPid getSophosPid() const;

            /*
             * Setters
             */
            void setSessionType(SessionType sessionType);
            void setEventType(EventType eventType);
            void setSubjectUserSid(const Common::EventTypes::UserSid& subjectUserSid);
            void setTargetUserSid(const Common::EventTypes::UserSid& targetUserSid);
            void setGroupId(unsigned long groupId);
            void setGroupName(const std::string& groupName);
            void setTimestamp(windows_timestamp_t timestamp);
            void setLogonId(login_id_t logonId);
            void setRemoteNetworkAccess(const Common::EventTypes::NetworkAddress& remoteNetworkAccess);
            void setSophosPid(const Common::EventTypes::SophosPid& processId);

            std::string toString() const override;

            /**
             * Takes in a event object as a capn byte string.
             * @param a objectAsString
             * @returns CredentialEvent object created from the capn byte string..
             */
            void fromString(const std::string& objectAsString);

        private:
            unsigned long m_groupId;
            std::string m_groupName;
            unsigned long long m_timestamp;
            unsigned long m_logonId;
            Common::EventTypes::NetworkAddress m_remoteNetworkAccess;
            Common::EventTypes::CredentialEvent::SessionType m_sessionType;
            Common::EventTypes::CredentialEvent::EventType m_eventType;
            Common::EventTypes::UserSid m_subjectUserSid;
            Common::EventTypes::UserSid m_targetUserSid;
            Common::EventTypes::SophosPid m_sophosPid;
        };

        Common::EventTypes::CredentialEvent createCredentialEvent(
            const Common::EventTypes::UserSid& sid,
            Common::EventTypes::CredentialEvent::EventType eventType);
    } // namespace EventTypes

} // namespace Common
