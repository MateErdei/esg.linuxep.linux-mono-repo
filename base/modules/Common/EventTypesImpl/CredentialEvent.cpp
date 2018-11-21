/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "CredentialEvent.h"

#include <Common/EventTypes/IEventException.h>
#include <Common/EventTypes/ICredentialEvent.h>
#include <Common/EventTypes/CommonEventData.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include <iostream>
#include <sstream>

namespace Common
{
    namespace EventTypesImpl
    {
        CredentialEvent::CredentialEvent()
        {

        }

        const std::string CredentialEvent::getEventTypeId() const
        {
            return "Credentials";
        }

        const Common::EventTypes::SessionType CredentialEvent::getSessionType() const
        {
            return m_sessionType;
        }

        const Common::EventTypes::EventType CredentialEvent::getEventType() const
        {
            return m_eventType;
        }

        const Common::EventTypes::UserSID CredentialEvent::getSubjectUserSID() const
        {
            return m_subjectUserID;
        }

        const Common::EventTypes::UserSID CredentialEvent::getTargetUserSID() const
        {
            return m_targetUserSID;
        }

        const unsigned long long CredentialEvent::getTimestamp() const
        {
            return m_timestamp;
        }

        const unsigned long CredentialEvent::getLogonID() const
        {
            return m_logonID;
        }

        const Common::EventTypes::NetworkAddress CredentialEvent::getRemoteNetworkAccess() const
        {
            return m_remoteNetworkAccess;
        }

        const unsigned long CredentialEvent::getGroupId() const
        {
            return m_groupID;
        }

        const std::string CredentialEvent::getGroupName() const
        {
            return m_groupName;
        }

        void CredentialEvent::setSessionType(Common::EventTypes::SessionType sessionType)
        {
            m_sessionType = sessionType;
        }

        void CredentialEvent::setEventType(Common::EventTypes::EventType eventType)
        {
            m_eventType = eventType;
        }
        void CredentialEvent::setSubjectUserSID(Common::EventTypes::UserSID subjectUserSID)
        {
            m_subjectUserID = subjectUserSID;
        }

        void CredentialEvent::setTargetUserSID(Common::EventTypes::UserSID targetUserSID)
        {
            m_targetUserSID = targetUserSID;
        }

        void CredentialEvent::setTimestamp(unsigned long long timestamp)
        {
            m_timestamp = timestamp;
        }

        void CredentialEvent::setLogonID(unsigned long logonID)
        {
            m_logonID = logonID;
        }

        void CredentialEvent::setRemoteNetworkAccess(Common::EventTypes::NetworkAddress remoteNetworkAccess)
        {
            m_remoteNetworkAccess = remoteNetworkAccess;
        }

        void CredentialEvent::setGroupId(unsigned long groupId)
        {
            m_groupID = groupId;
        }

        void CredentialEvent::setGroupName(std::string& groupName)
        {
            m_groupName = groupName;
        }

        Sophos::Journal::CredentialsEvent::SessionType CredentialEvent::convertToCapnSessionType(Common::EventTypes::SessionType sessionType)
        {
            switch(sessionType)
            {
                case Common::EventTypes::SessionType::interactive:
                    return Sophos::Journal::CredentialsEvent::SessionType::INTERACTIVE;
                case Common::EventTypes::SessionType::network:
                    return Sophos::Journal::CredentialsEvent::SessionType::NETWORK;
                case Common::EventTypes::SessionType::networkInteractive:
                    return Sophos::Journal::CredentialsEvent::SessionType::NETWORK_INTERACTIVE;
                default:
                    throw Common::EventTypes::IEventException("Common::EventTypes::SessionType, contained unknown type");
            }
        }

        Sophos::Journal::CredentialsEvent::EventType CredentialEvent::convertToCapnEventType(Common::EventTypes::EventType eventType)
        {
            switch(eventType)
            {
                case Common::EventTypes::EventType::authFailure:
                    return Sophos::Journal::CredentialsEvent::EventType::AUTH_FAILURE;
                case Common::EventTypes::EventType::authSuccess:
                    return Sophos::Journal::CredentialsEvent::EventType::AUTH_SUCCESS;
                case Common::EventTypes::EventType::created:
                    return Sophos::Journal::CredentialsEvent::EventType::CREATED;
                case Common::EventTypes::EventType::deleted:
                    return Sophos::Journal::CredentialsEvent::EventType::DELETED;
                case Common::EventTypes::EventType::passwordChange:
                    return Sophos::Journal::CredentialsEvent::EventType::PASSWORD_CHANGE;
                case Common::EventTypes::EventType::membershipChange:
                    return Sophos::Journal::CredentialsEvent::EventType::MEMBERSHIP_CHANGE;
                default:
                    throw Common::EventTypes::IEventException("Common::EventTypes::EventType, contained unknown type");
            }
        }

        Common::EventTypes::SessionType CredentialEvent::convertFromCapnSessionType(Sophos::Journal::CredentialsEvent::SessionType sessionType)
        {
            switch(sessionType)
            {
                case Sophos::Journal::CredentialsEvent::SessionType::INTERACTIVE:
                    return Common::EventTypes::SessionType::interactive;
                case Sophos::Journal::CredentialsEvent::SessionType::NETWORK:
                    return Common::EventTypes::SessionType::network;
                case Sophos::Journal::CredentialsEvent::SessionType::NETWORK_INTERACTIVE:
                    return Common::EventTypes::SessionType::networkInteractive;
                default:
                    throw Common::EventTypes::IEventException("Sophos::Journal::CredentialsEvent::SessionType, contained unknown type");

            }
        }

        Common::EventTypes::EventType CredentialEvent::convertFromCapnEventType(Sophos::Journal::CredentialsEvent::EventType eventType)
        {
            switch(eventType)
            {
                case Sophos::Journal::CredentialsEvent::EventType::AUTH_FAILURE:
                    return Common::EventTypes::EventType::authFailure;
                case Sophos::Journal::CredentialsEvent::EventType::AUTH_SUCCESS:
                    return Common::EventTypes::EventType::authSuccess;
                case Sophos::Journal::CredentialsEvent::EventType::CREATED:
                    return Common::EventTypes::EventType::created;
                case Sophos::Journal::CredentialsEvent::EventType::DELETED:
                    return Common::EventTypes::EventType::deleted;
                case Sophos::Journal::CredentialsEvent::EventType::PASSWORD_CHANGE:
                    return Common::EventTypes::EventType::passwordChange;
                case Sophos::Journal::CredentialsEvent::EventType::MEMBERSHIP_CHANGE:
                    return Common::EventTypes::EventType::membershipChange;
                default:
                    throw Common::EventTypes::IEventException("Sophos::Journal::CredentialsEvent::EventType, contained unknown type");
            }


        }

        std::string CredentialEvent::toString()
        {

            ::capnp::MallocMessageBuilder message;
            Sophos::Journal::CredentialsEvent::Builder credentialsEvent = message.initRoot<Sophos::Journal::CredentialsEvent>();

            credentialsEvent.setSessionType(convertToCapnSessionType(m_sessionType));
            credentialsEvent.setEventType(convertToCapnEventType(m_eventType));

            Sophos::Journal::UserSID::Builder subjectUserId = credentialsEvent.initSubjectUserSID();
            subjectUserId.setUsername(m_subjectUserID.username);

            subjectUserId.setDomain(m_subjectUserID.domain);
            credentialsEvent.setSubjectUserSID(subjectUserId);

            Sophos::Journal::UserSID::Builder targetUserId = credentialsEvent.initTargetUserSID();
            targetUserId.setUsername(m_targetUserSID.username);

            targetUserId.setDomain(m_targetUserSID.domain);
            credentialsEvent.setTargetUserSID(targetUserId);


            credentialsEvent.setTimestamp(m_timestamp);
            credentialsEvent.setLogonID(m_logonID);
            Sophos::Journal::NetworkAddress::Builder networkAddress = credentialsEvent.initRemoteNetworkAddress();
            networkAddress.setAddress(m_remoteNetworkAccess.address);
            credentialsEvent.setUserGroupID(m_groupID);
            credentialsEvent.setUserGroupName(m_groupName);

            // Convert to byte string
            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        Common::EventTypes::ICredentialEvent* CredentialEvent::fromString(std::string& objectAsString)
        {

            const kj::ArrayPtr<const capnp::word> view(
                    reinterpret_cast<const capnp::word*>(&(*std::begin(objectAsString))),
                    reinterpret_cast<const capnp::word*>(&(*std::end(objectAsString))));

            capnp::FlatArrayMessageReader message(view);
            Sophos::Journal::CredentialsEvent::Reader credentialsEvent = message.getRoot<Sophos::Journal::CredentialsEvent>();

            Common::EventTypesImpl::CredentialEvent event;

            event.setSessionType(convertFromCapnSessionType(credentialsEvent.getSessionType()));
            event.setEventType(convertFromCapnEventType(credentialsEvent.getEventType()));

            credentialsEvent.getUserGroupName().cStr();

            std::string groupName(credentialsEvent.getUserGroupName());
            event.setGroupName(groupName);

            event.setGroupId(credentialsEvent.getUserGroupID());
            event.setTimestamp(credentialsEvent.getTimestamp());
            event.setLogonID(credentialsEvent.getLogonID());

            Common::EventTypes::UserSID subjectUserId;
            subjectUserId.username = credentialsEvent.getSubjectUserSID().getUsername();
            subjectUserId.domain = credentialsEvent.getSubjectUserSID().getDomain();
            event.setSubjectUserSID(subjectUserId);

            Common::EventTypes::UserSID targetUserId;
            targetUserId.username = credentialsEvent.getTargetUserSID().getUsername();
            targetUserId.domain = credentialsEvent.getTargetUserSID().getDomain();
            event.setTargetUserSID(targetUserId);

            Common::EventTypes::NetworkAddress networkAddress;
            networkAddress.address = credentialsEvent.getRemoteNetworkAddress().getAddress();
            event.setRemoteNetworkAccess(networkAddress);

            return &event;
        }
    }
}