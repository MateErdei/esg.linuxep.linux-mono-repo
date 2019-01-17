/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "CommonEventData.h"
#include "IEventType.h"


namespace Common
{
    namespace EventTypes
    {
    class ProcessEvent : public Common::EventTypes::IEventType
        {
        public:

        enum EventType
        {
            start = 0,
            end = 1,
        };
            ProcessEvent() = default;

            ~ProcessEvent() = default;

            const EventType getEventType() const;

            // Gets the sophosPid which is the PID concatenated with timestamp of when the process started.
            const Common::EventTypes::SophosPid getSophosPid() const;

            // A SophosPid is the PID concatenated with timestamp of when the process started.
            // This is the parent process SophosPID
            const Common::EventTypes::SophosPid getParentSophosPid() const;

            // Gets the sophosPid which is the TID (thread ID) concatenated with timestamp of when the process started.
            const Common::EventTypes::SophosTid getParentSophosTid() const;

            const std::string getEventTypeId() const override;
            const unsigned long long getEndTime() const;
            const Common::EventTypes::OptionalUInt64 getFileSize() const ;

            // Process flags, we don't know what these will be on Linux, will likely just be 0.
            const std::uint32_t getFlags() const;

            // On linux this is the process namespace.
            const std::uint32_t getSessionId() const;

            // On linux this is the UID, on windows SID (Security Identifier) has a specific meaning.
            const std::string getSid() const ;
            const Common::EventTypes::Pathname getPathname() const;

            // Command line of running process - e.g. what you get when you run a ps on Linux.
            const std::string getCmdLine() const;
            const std::string getSha256() const;
            const std::string getSha1() const;

            void setEventType(const EventType eventType);
            void setSophosPid(Common::EventTypes::SophosPid sophosPid);
            void setParentSophosPid(Common::EventTypes::SophosPid parentSophosPid);
            void setParentSophosTid(Common::EventTypes::SophosTid parentSophosTid);
            void setEndTime(const unsigned long long endTime);
            void setFileSize(const Common::EventTypes::OptionalUInt64 fileSize);
            void setFlags(const std::uint32_t flags);
            void setSessionId(const std::uint32_t sessionId);
            void setSid(const std::string sid) ;
            void setPathname(const Common::EventTypes::Pathname pathname);
            void setCmdLine(std::string cmdLine);
            void setSha256(const std::string sha256);
            void setSha1(const std::string sha1);

            std::string toString() const override;

            /**
            * Takes in a event object as a capn byte string.
            * @param a objectAsString
            * @returns CredentialEvent object created from the capn byte string..
            */
            void fromString(const std::string& objectAsString);

        private:
            Common::EventTypes::ProcessEvent::EventType m_eventType;

            // A SophosPid is the PID concatenated with timestamp of when the process started.
            Common::EventTypes::SophosPid m_sophosPid;

            // A SophosPid is the PID concatenated with timestamp of when the process started.
            // This is the parent process SophosPID
            Common::EventTypes::SophosPid m_parentSophosPid;

            // A sophosTid is the TID (thread ID) concatenated with timestamp of when the process started.
            Common::EventTypes::SophosTid m_parentSophosTid;
            unsigned long long m_endTime;
            Common::EventTypes::OptionalUInt64 m_fileSize;

            // Process flags, we don't know what these will be on Linux, will likely just be 0.
            std::uint32_t m_flags;

            // On linux this is the process namespace.
            std::uint32_t m_sessionId;

            // On linux this is the UID, on windows SID (Security Identifier) has a specific meaning.
            std::string m_sid;
            Common::EventTypes::Pathname m_pathname;

            // Command line of running process - e.g. what you get when you run a ps on Linux.
            std::string m_cmdLine;
            std::string m_sha256;
            std::string m_sha1;
        };

        Common::EventTypes::ProcessEvent createProcessEvent(Common::EventTypes::ProcessEvent::EventType eventType);
    }

}
