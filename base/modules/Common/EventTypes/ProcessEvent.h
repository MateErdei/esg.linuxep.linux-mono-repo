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

            /**
             * return the event type of this process event, i.e. if it's a start or end event.
             * @return
             */
            const EventType getEventType() const;

            /**
             * Gets the SophosPid of this process which is the PID concatenated with timestamp of when the process started.
             * @return
             */
            const Common::EventTypes::SophosPid getSophosPid() const;

            /**
             * This is the parent process SophosPID as explained above
             * @return
             */
            const Common::EventTypes::SophosPid getParentSophosPid() const;

            /**
             * Gets the sophosPid which is the TID (thread ID) concatenated with timestamp of when the process started.
             * @return
             */
            const Common::EventTypes::SophosTid getParentSophosTid() const;

            /**
             * return the ID of the event type.
             * @return
             */
            const std::string getEventTypeId() const override;

            /**
             * Return the time the process ended, this may not be present if it's a start event.
             * @return
             */
            const unsigned long long getEndTime() const;

            /**
             * Return an optional filesize of the binary
             * @return
             */
            const Common::EventTypes::OptionalUInt64 getFileSize() const ;

            /**
             * Process flags, we don't know what these will be on Linux, will likely just be 0.
             * @return
             */
            const std::uint32_t getFlags() const;

            /**
             * On linux this is the process namespace.
             * @return
             */
            const std::uint32_t getSessionId() const;

            /**
             * On linux this is the UID, on windows SID (Security Identifier) has a specific meaning.
             * @return
             */
            const std::string getSid() const ;

            /**
             * Return a struct that holds information about the path of the binary file.
             * @return
             */
            const Common::EventTypes::Pathname getPathname() const;

            /**
             * Command line of running process - e.g. what you see when you run a ps on Linux.
             * @return
             */
            const std::string getCmdLine() const;

            /**
             * Return the SHA256 of the executable.
             * @return
             */
            const std::string getSha256() const;

            /**
             * Return the SHA1 of the executable.
             * @return
             */
            const std::string getSha1() const;

            /**
             * Sets the type of process event, either it's a process starting or ending.
             * @param eventType
             */
            void setEventType(const EventType eventType);

            /**
             * Sets sophosPid which is the Sophos Process ID of this process.
             * A Sophos Process ID is the concatenation of the PID and timestamp of the start of the process.
             * @param sophosPid
             */
            void setSophosPid(Common::EventTypes::SophosPid sophosPid);

            /**
             * Sets parentSophosPid which is the Sophos Process ID of this process' parent.
             * @param parentSophosPid
             */
            void setParentSophosPid(Common::EventTypes::SophosPid parentSophosPid);

            /**
             * Sets parentSophosTid which is the Sophos Thread ID of this process' parent.
             * @param parentSophosTid
             */
            void setParentSophosTid(Common::EventTypes::SophosTid parentSophosTid);

            /**
             * Sets the process end time property of this ProcessEvent object
             * @param endTime
             */
            void setEndTime(const unsigned long long endTime);

            /**
             * Sets the filesize, in bytes, property of this ProcessEvent object
             * @param fileSize
             */
            void setFileSize(const Common::EventTypes::OptionalUInt64 fileSize);

            /**
             * Sets the flags property of this ProcessEvent object
             * @param flags
             */
            void setFlags(const std::uint32_t flags);

            /**
             * Sets the session ID property of this ProcessEvent object
             * @param sessionId
             */
            void setSessionId(const std::uint32_t sessionId);

            /**
             * Sets the sid property of this ProcessEvent object
             * @param sid
             */
            void setSid(const std::string sid) ;

            /**
             * Sets the pathname property of this ProcessEvent object
             * @param pathname
             */
            void setPathname(const Common::EventTypes::Pathname pathname);

            /**
             * Sets the command line property of this ProcessEvent object
             * @param cmdLine
             */
            void setCmdLine(std::string cmdLine);

            /**
             * Sets the SHA256 property of this ProcessEvent object
             * @param sha256
             */
            void setSha256(const std::string sha256);

            /**
             * Sets the SHA1 property of this ProcessEvent object
             * @param sha1
             */
            void setSha1(const std::string sha1);

            /**
             * Turns this ProcessEvent object into a capn byte string.
             * @return a capn byte string of the ProcessEvent.
             */
            std::string toString() const override;

            /**
            * Takes in a event object as a capn byte string.
            * @param an objectAsString
            * @returns ProcessEvent object created from the capn byte string.
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
