///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef MANAGEMENTAGENT_PLUGINCOMMUNICATIONIMPL_STATUSCACHE_H
#define MANAGEMENTAGENT_PLUGINCOMMUNICATIONIMPL_STATUSCACHE_H


#include <string>
#include <map>

namespace ManagementAgent
{
    namespace StatusReceiverImpl
    {
        class IStatusCache
        {

        public:
            virtual ~IStatusCache() = default;
            /**
             *
             * @param appid
             * @param statusForComparison
             * @return True if the status has changed
             */
            virtual bool statusChanged(const std::string& appid, const std::string& statusForComparison) = 0;
        };

        class StatusCache : public virtual IStatusCache
        {
        public:
            ~StatusCache() override = default;
            bool statusChanged(const std::string& appid, const std::string& statusForComparison) override;
        private:
            /**
             * Possibly need to convert to on-disk, or use on-disk as well
             */
            std::map<std::string, std::string> m_statusCache;

            /**
             * Update m_statusCache and return true.
             * @param appid
             * @param statusForComparison
             * @return true
             */
            bool updateStatus(const std::string& appid, const std::string& statusForComparison);

        };
    }
}


#endif //MANAGEMENTAGENT_PLUGINCOMMUNICATIONIMPL_STATUSCACHE_H
