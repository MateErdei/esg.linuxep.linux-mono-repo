/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

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
            const Common::EventTypes::SophosPid getSophosPid() const;
            const Common::EventTypes::SophosPid getParentSophosPid() const;
            const Common::EventTypes::SophosTid getParentSophosTid() const;
            const std::string getEventTypeId() const override;
            const unsigned long long getEndTime() const;
            const Common::EventTypes::OptionalUInt64 getFileSize() const ;
            const std::uint32_t getFlags() const;
            const std::uint32_t getSessionId() const;
            const std::string getSid() const ;
            const Common::EventTypes::Pathname getPathname() const;
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
            Common::EventTypes::SophosPid m_sophosPid;
            Common::EventTypes::SophosPid m_parentSophosPid;
            Common::EventTypes::SophosTid m_parentSophosTid;
            unsigned long long m_endTime;
            Common::EventTypes::OptionalUInt64 m_fileSize;
            std::uint32_t m_flags;

            // On linux this is the process namespace.
            std::uint32_t m_sessionId;

            // On linux this is the UID, on windows SID (Security Identifier) has a specific meaning.
            std::string m_sid;
            Common::EventTypes::Pathname m_pathname;
            std::string m_cmdLine;
            std::string m_sha256;
            std::string m_sha1;
        };

        Common::EventTypes::ProcessEvent createProcessEvent(Common::EventTypes::ProcessEvent::EventType eventType);
    }

}
