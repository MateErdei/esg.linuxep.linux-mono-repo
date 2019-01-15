/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ProcessEvent.h"
#include "EventStrings.h"

#include <Common/EventTypes/IEventException.h>
#include <Common/EventTypes/CommonEventData.h>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <Process.capnp.h>

#include <iostream>
#include <sstream>

namespace
{

    Sophos::Journal::ProcessEvent::EventType convertToCapnEventType(Common::EventTypes::ProcessEvent::EventType eventType)
    {
        switch(eventType)
        {
            case Common::EventTypes::ProcessEvent::EventType::start:
                return Sophos::Journal::ProcessEvent::EventType::START;
            case Common::EventTypes::ProcessEvent::EventType::end:
                return Sophos::Journal::ProcessEvent::EventType::END;
            default:
                throw Common::EventTypes::IEventException("Common::EventTypes::EventType, contained unknown type");
        }
    }

    Common::EventTypes::ProcessEvent::EventType convertFromCapnEventType(Sophos::Journal::ProcessEvent::EventType eventType)
    {
        switch(eventType)
        {
            case Sophos::Journal::ProcessEvent::EventType::START:
                return Common::EventTypes::ProcessEvent::EventType::start;
            case Sophos::Journal::ProcessEvent::EventType::END:
                return Common::EventTypes::ProcessEvent::EventType::end;
            default:
                throw Common::EventTypes::IEventException("Sophos::Journal::ProcessEvent::EventType, contained unknown type");
        }
    }
}

Common::EventTypes::ProcessEvent Common::EventTypes::createProcessEvent(Common::EventTypes::ProcessEvent::EventType eventType)
{
    Common::EventTypes::ProcessEvent event = ProcessEvent();
    event.setEventType(eventType);

    return event;
}

namespace Common
{
    namespace EventTypes
    {

        std::string ProcessEvent::toString() const
        {

            ::capnp::MallocMessageBuilder message;
            Sophos::Journal::ProcessEvent::Builder processEvent = message.initRoot<Sophos::Journal::ProcessEvent>();

            processEvent.setEventType(convertToCapnEventType(m_eventType));

            processEvent.setEndTime(m_endTime);
            processEvent.setFlags(m_flags);
            processEvent.setSessionId(m_sessionId);
            processEvent.setCmdLine(m_cmdLine);

            processEvent.getFileSize().setValue(m_fileSize.value);
            processEvent.getPathname().setFlags(m_pathname.flags);
            processEvent.getPathname().setFileSystemType(m_pathname.fileSystemType);
            processEvent.getPathname().setDriveLetter(m_pathname.driveLetter);
            processEvent.getPathname().setPathname(m_pathname.pathname);
            processEvent.getPathname().getOpenName().setLength(m_pathname.openName.length);
            processEvent.getPathname().getOpenName().setOffset(m_pathname.openName.offset);
            processEvent.getPathname().getVolumeName().setLength(m_pathname.volumeName.length);
            processEvent.getPathname().getVolumeName().setOffset(m_pathname.volumeName.offset);
            processEvent.getPathname().getShareName().setLength(m_pathname.shareName.length);
            processEvent.getPathname().getShareName().setOffset(m_pathname.shareName.offset);
            processEvent.getPathname().getExtensionName().setLength(m_pathname.extensionName.length);
            processEvent.getPathname().getExtensionName().setOffset(m_pathname.extensionName.offset);
            processEvent.getPathname().getStreamName().setLength(m_pathname.streamName.length);
            processEvent.getPathname().getStreamName().setOffset(m_pathname.streamName.offset);
            processEvent.getPathname().getFinalComponentName().setLength(m_pathname.finalComponentName.length);
            processEvent.getPathname().getFinalComponentName().setOffset(m_pathname.finalComponentName.offset);
            processEvent.getPathname().getParentDirName().setLength(m_pathname.parentDirName.length);
            processEvent.getPathname().getParentDirName().setOffset(m_pathname.parentDirName.offset);

            // Convert to byte string
            kj::Array<capnp::word> dataArray = capnp::messageToFlatArray(message);
            kj::ArrayPtr<kj::byte> bytes = dataArray.asBytes();
            std::string dataAsString(bytes.begin(), bytes.end());

            return dataAsString;
        }

        void ProcessEvent::fromString(const std::string& objectAsString)
        {

            if(objectAsString.empty())
            {
                throw Common::EventTypes::IEventException("Invalid capn byte string, string is empty");
            }

            try
            {
                const kj::ArrayPtr<const capnp::word> view(
                        reinterpret_cast<const capnp::word*>(&(*std::begin(objectAsString))),
                        reinterpret_cast<const capnp::word*>(&(*std::end(objectAsString))));


                capnp::FlatArrayMessageReader message(view);
                Sophos::Journal::ProcessEvent::Reader processEvent = message.getRoot<Sophos::Journal::ProcessEvent>();

                setEventType(convertFromCapnEventType(processEvent.getEventType()));
                setEndTime(processEvent.getEndTime());
                setFlags(processEvent.getFlags());
                setSessionId(processEvent.getSessionId());
                setCmdLine(processEvent.getCmdLine());

                Common::EventTypes::OptionalUInt64 fileSize {processEvent.getFileSize().getValue()};
                setFileSize(fileSize);

                Common::EventTypes::Pathname pathname;
                pathname.flags = processEvent.getPathname().getFlags();
                pathname.fileSystemType = processEvent.getPathname().getFileSystemType();
                pathname.driveLetter = processEvent.getPathname().getDriveLetter();
                pathname.pathname = processEvent.getPathname().getPathname();
                pathname.openName.length = processEvent.getPathname().getOpenName().getLength();
                pathname.openName.offset = processEvent.getPathname().getOpenName().getOffset();
                pathname.volumeName.length = processEvent.getPathname().getVolumeName().getLength();
                pathname.volumeName.offset = processEvent.getPathname().getVolumeName().getOffset();
                pathname.shareName.length = processEvent.getPathname().getShareName().getLength();
                pathname.shareName.offset = processEvent.getPathname().getShareName().getOffset();
                pathname.extensionName.length = processEvent.getPathname().getExtensionName().getLength();
                pathname.extensionName.offset = processEvent.getPathname().getExtensionName().getOffset();
                pathname.streamName.length = processEvent.getPathname().getStreamName().getLength();
                pathname.streamName.offset = processEvent.getPathname().getStreamName().getOffset();
                pathname.finalComponentName.length = processEvent.getPathname().getFinalComponentName().getLength();
                pathname.finalComponentName.offset = processEvent.getPathname().getFinalComponentName().getOffset();
                pathname.parentDirName.length = processEvent.getPathname().getParentDirName().getLength();
                pathname.parentDirName.offset = processEvent.getPathname().getParentDirName().getOffset();
                setPathname(pathname);
            }
            catch(std::exception& ex)
            {
                std::stringstream errorMessage;
                errorMessage << "Error: failed to process capn ProcessEvent string, " << ex.what();
                throw Common::EventTypes::IEventException(errorMessage.str());
            }
        }

        const Common::EventTypes::ProcessEvent::EventType EventTypes::ProcessEvent::getEventType() const
        {
            return m_eventType;
        }

        const std::string ProcessEvent::getEventTypeId() const
        {
            return Common::EventTypes::ProcessEventName;
        }

        const unsigned long long ProcessEvent::getEndTime() const
        {
            return m_endTime;
        }

        const Common::EventTypes::OptionalUInt64 ProcessEvent::getFileSize() const
        {
            return m_fileSize;
        }

        const std::uint32_t ProcessEvent::getFlags() const
        {
            return m_flags;
        }

        const std::uint32_t ProcessEvent::getSessionId() const
        {
            return m_sessionId;
        }

//        const std::string ProcessEvent::getSId() const
//        {
//            return m_sid;
//        }

        const Common::EventTypes::Pathname ProcessEvent::getPathname() const
        {
            return m_pathname;
        }

        const std::string ProcessEvent::getCmdLine() const
        {
            return m_cmdLine;
        }

        void ProcessEvent::setEventType(const ProcessEvent::EventType eventType)
        {
            m_eventType = eventType;
        }

        void ProcessEvent::setEndTime(const unsigned long long endTime)
        {
            m_endTime = endTime;
        }

        void ProcessEvent::setFileSize(const Common::EventTypes::OptionalUInt64 fileSize)
        {
            m_fileSize = fileSize;
        }

        void ProcessEvent::setFlags(const std::uint32_t flags)
        {
            m_flags = flags;
        }

        void ProcessEvent::setSessionId(const std::uint32_t sessionId)
        {
            m_sessionId = sessionId;
        }

        void ProcessEvent::setPathname(const Common::EventTypes::Pathname pathname)
        {
            m_pathname = pathname;
        }

        void ProcessEvent::setCmdLine(const std::string cmdLine)
        {
            m_cmdLine = cmdLine;
        }
    }
}