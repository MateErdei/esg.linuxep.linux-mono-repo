/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IEventType.h"
#include "Common/EventTypes/CommonEventData.h"
#include <string>
#include <memory>
#include <vector>

namespace Common
{
    namespace EventTypes
    {
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

        class ICredentialEvent : public IEventType
        {
        public:

            /**
             * Gets the SessionType of the event
             * @return the SessionType
             */
            virtual const SessionType getSessionType() const = 0;

            /**
             * Gets the EventType of the event
             * @return the EventType
             */
            virtual const EventType getEventType() const = 0;

            /**
             * Gets the SubjectUserSID of the event
             * @return the SubjectUserSID
             */
            virtual const UserSID getSubjectUserSID() const = 0;

            /**
             * Gets the TargetUserSID of the event
             * @return the TargetUserSID
             */
            virtual const UserSID getTargetUserSID() const = 0;

            /**
             * Gets the GroupId of the event
             * @return the GroupId of the user who triggered the event
             */
            virtual const unsigned long getGroupId() const = 0;

            /**
             * Gets the GroupName of the event
             * @return the GroupName of the user who triggered the event
             */
            virtual const std::string getGroupName() const = 0;

            /**
             * Gets the Timestamp of the event
             * @return the Timestamp of the event
             */
            virtual const unsigned long long getTimestamp() const = 0;

            /**
             * Gets the LogonID of the event
             * @return the LogonID of the event
             */
            virtual const unsigned long getLogonID() const = 0;

            /**
             * Gets the NetworkAddress of the event
             * @return the NetworkAddress of the event
             */
            virtual const NetworkAddress getRemoteNetworkAccess() const = 0;

            /**
             * Takes in a SessionType and sets the credentialsEvent object SessionType to it
             * @param a SessionType
             */
            virtual void setSessionType(SessionType sessionType) = 0;

            /**
             * Takes in a EventType and sets the credentialsEvent object EventType to it
             * @param a EventType
             */
            virtual void setEventType(EventType eventType) = 0;

            /**
             * Takes in a UserSID and sets the credentialsEvent object SubjectUserSID to it
             * @param a subjectUserSID
             */
            virtual void setSubjectUserSID(UserSID subjectUserSID) = 0;

            /**
             * Takes in a UserSID and sets the credentialsEvent object TargetUserSID to it
             * @param a targetUserSID
             */
            virtual void setTargetUserSID(UserSID targetUserSID) = 0;

            /**
             * Takes in a unsigned long and sets the credentialsEvent groupId to it
             * @param a groupId
             */
            virtual void setGroupId(unsigned long groupId) = 0;

            /**
             * Takes in a string and sets the credentialsEvent groupName to it
             * @param a groupName
             */
            virtual void setGroupName(std::string& groupName) = 0;

            /**
             * Takes in a UInt64 and sets the credentialsEvent timestamp to it
             * @param a timestamp
             */
            virtual void setTimestamp(unsigned long long timestamp) = 0;

            /**
             * Takes the Id of the user who is logged in, or attempted to log in.
             * @param a logonID
             */
            virtual void setLogonID(unsigned long logonID) = 0;

            /**
             * Takes in a NetworkAddress object and sets the credentialsEvent remoteNetworkAccess to it
             * @param a remoteNetworkAccess
             */
            virtual void setRemoteNetworkAccess(NetworkAddress remoteNetworkAccess) = 0;

            /**
            * Takes in a event object as a capn byte string.
            * @param a objectAsString
            * @returns CredentialEvent object created from the capn byte string..
            */
            virtual ICredentialEvent* fromString(std::string& objectAsString) = 0;

        };
    }
}
