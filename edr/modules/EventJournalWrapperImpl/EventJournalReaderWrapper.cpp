/***********************************************************************************************

Copyright 2021 Sophos Limited. All rights reserved.

***********************************************************************************************/
#include "EventJournalReaderWrapper.h"

#include "EventJournalTimeUtils.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <capnp/message.h>
#include <capnp/serialize.h>

#include <cstddef>
#include <cstring>
#include <fstream>
#include <iomanip>

#include <Event.capnp.h>
#include <Journal.h>

namespace Common
{
    namespace EventJournalWrapper
    {
        static std::string getSubjectName(Subject subject)
        {
            switch (subject)
            {
                case Subject::Detections: return "Detections";
                default: return "";
            }
        }

        Reader::Reader()
        {
            m_helper = Sophos::Journal::GetHelper();
        }

        Reader::Reader(const std::string& location) : m_location(location)
        {
            m_helper = Sophos::Journal::GetHelper(m_location);
        }

        std::string Reader::getCurrentJRLForId(const std::string& idFilePath)
        {
            std::string jrl("");
            auto filesystem = Common::FileSystem::fileSystem();

            if(filesystem->isFile(idFilePath))
            {
                jrl = filesystem->readFile(idFilePath);
            }
            return jrl;
        }
        void Reader::updateJrl(const std::string& idFilePath, const std::string& jrl)
        {
            if(idFilePath.empty())
            {
                return;
            }
            auto filesystem = Common::FileSystem::fileSystem();

            if(filesystem->isFile(idFilePath))
            {
                filesystem->removeFile(idFilePath);
            }

            filesystem->writeFile(idFilePath, jrl);
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, const std::string& jrl)
        {
            return getEntries(subjectFilter, jrl, 0,0,0);
        }

        std::vector<Entry> Reader::getEntries(
            std::vector<Subject> subjectFilter,
            uint32_t limit,
            uint64_t startTime,
            uint64_t endTime)
        {
            return getEntries(subjectFilter, "", limit,startTime, endTime);
        }

        std::vector<Entry> Reader::getEntries(
            std::vector<Subject> subjectFilter,
            const std::string& jrl,
            uint32_t limit,
            uint64_t startTime,
            uint64_t endTime
            )
        {
            std::vector<Entry> entries;
            (void)limit;
            (void)startTime;
            (void)endTime;

            std::vector<std::string> subjects;
            for (auto s : subjectFilter)
            {
                subjects.push_back(getSubjectName(s));
            }
            std::shared_ptr <Sophos::Journal::ViewInterface> view;
            if(!jrl.empty())
            {
                view = m_helper->GetJournalView(subjects, jrl);
            }
            else
            {
                view = m_helper->GetJournalView(subjects, Sophos::Journal::MinTime);
            }

            if (!view)
            {
                throw std::runtime_error("");
            }

            for (auto it = view->cbegin(); it != view->cend(); ++it)
            {
                auto entry = *it;
                if (!entry)
                {
                    continue;
                }

                Entry e;
                e.producerUniqueID = entry->GetProducerUniqueId();
                e.timestamp = getWindowsFileTime(entry->GetTimestamp());
                e.jrl = it.GetJournalResourceLocator();
                e.data.resize(entry->GetDataSize());
                memcpy(e.data.data(), entry->GetData(), entry->GetDataSize());
                entries.push_back(e);
            }

            return entries;
        }

        std::pair<bool, Detection> Reader::decode(const std::vector<uint8_t>& data)
        {
            Detection detection;
            auto array = kj::ArrayPtr<const capnp::word>(reinterpret_cast<const capnp::word*>(data.data()), data.size() / sizeof(capnp::word));
            capnp::FlatArrayMessageReader reader(array);
            auto event = reader.getRoot<Sophos::Journal::Event>();

            if (!event.isJson())
            {
                LOGWARN("unexpected detection event " << event.which());
                return std::make_pair(false, detection);
            }

            auto json = event.getJson();
            detection.subType = json.getSubType();
            detection.data = json.getData();

            return std::make_pair(true, detection);
        }

    } // namespace EventJournalWrapper
} // namespace Common
