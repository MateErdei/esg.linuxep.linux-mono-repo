/***********************************************************************************************

Copyright 2021 Sophos Limited. All rights reserved.

***********************************************************************************************/
#include "EventJournalReaderWrapper.h"
#include "EventJournalTimeUtils.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <cstring>
#include <fstream>


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
            std::optional<Sophos::Journal::ViewSettings> settings = Sophos::Journal::ViewSettings();
            settings.value()[Sophos::Journal::Settings::FlushDelayKey] = "false";
            settings.value()[Sophos::Journal::Settings::FlushDelayIntervalKey] = "0";
            settings.value()[Sophos::Journal::Settings::JournalLocationKey] = m_location;
            m_helper = Sophos::Journal::GetHelper(settings);
        }

        std::string Reader::getCurrentJRLForId(const std::string& idFilePath)
        {
            std::string jrl("");
            auto filesystem = Common::FileSystem::fileSystem();

            if (filesystem->isFile(idFilePath))
            {
                jrl = filesystem->readFile(idFilePath);
            }
            return jrl;
        }
        void Reader::updateJrl(const std::string& idFilePath, const std::string& jrl)
        {
            if (idFilePath.empty())
            {
                return;
            }
            auto filesystem = Common::FileSystem::fileSystem();

            if (filesystem->isFile(idFilePath))
            {
                filesystem->removeFile(idFilePath);
            }

            filesystem->writeFile(idFilePath, jrl);
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, uint64_t startTime, uint64_t endTime, uint32_t limit, bool& moreAvailable)
        {
            return getEntries(subjectFilter, "", startTime, endTime, limit, moreAvailable);
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, const std::string& jrl, uint32_t limit, bool& moreAvailable)
        {
            return getEntries(subjectFilter, jrl, 0, 0, limit, moreAvailable);
        }

        std::vector<Entry> Reader::getEntries(std::vector<Subject> subjectFilter, const std::string& jrl,  uint64_t startTime, uint64_t endTime, uint32_t limit, bool& moreAvailable)
        {
            std::vector<Entry> entries;

            std::vector<std::string> subjects;
            for (auto s : subjectFilter)
            {
                subjects.push_back(getSubjectName(s));
            }
            std::shared_ptr <Sophos::Journal::ViewInterface> view;

            if (!jrl.empty())
            {
                view = m_helper->GetJournalView(subjects, jrl);
            }
            else
            {
                if (endTime == 0)
                {
                    view = m_helper->GetJournalView(
                        subjects,
                        EventJournalWrapper::getFileTimeFromWindowsTime(
                            UtilityImpl::TimeUtils::EpochToWindowsFileTime(startTime)));
                }
                else
                {
                    view = m_helper->GetJournalView(
                        subjects,
                        EventJournalWrapper::getFileTimeFromWindowsTime(
                            UtilityImpl::TimeUtils::EpochToWindowsFileTime(startTime)),
                        EventJournalWrapper::getFileTimeFromWindowsTime(
                                UtilityImpl::TimeUtils::EpochToWindowsFileTime(endTime)));
                }
            }

            if (!view)
            {
                throw std::runtime_error("");
            }

            uint32_t count = 0;
            bool more = false;
            for (auto it = view->cbegin(); it != view->cend(); ++it)
            {
                auto entry = *it;
                if (!entry)
                {
                    continue;
                }
                if (limit && (count >= limit))
                {
                    more = true;
                    break;
                }

                Entry e;
                e.producerUniqueID = entry->GetProducerUniqueId();
                e.timestamp = UtilityImpl::TimeUtils::WindowsFileTimeToEpoch(getWindowsFileTime(entry->GetTimestamp()));
                e.jrl = it.GetJournalResourceLocator();
                e.data.resize(entry->GetDataSize());
                memcpy(e.data.data(), entry->GetData(), entry->GetDataSize());
                entries.push_back(e);

                count++;
            }
            moreAvailable = more;
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
