/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "SophosString.h"

namespace Common
{
    namespace EventTypes
    {
        SophosString::SophosString(const std::string& str) :
        m_string(str)
        {
            limitStringSize();
        }

        SophosString::SophosString(const char * str) :
        m_string(str)
        {
            limitStringSize();
        }

        SophosString::SophosString(const SophosString& str) :
                m_string(str.str())
        {
            limitStringSize();
        }

        SophosString::SophosString(std::string&& str) :
        m_string(str)
        {
            limitStringSize();
        }

        SophosString::operator std::string() const
        {
            return m_string;
        }

        SophosString& SophosString::operator=(const std::string& rhs)
        {
            m_string = rhs;
            return *this;
        }

        SophosString& SophosString::operator=(const SophosString& rhs)
        {
            m_string = rhs.m_string;
            return *this;
        }

        SophosString& SophosString::operator=(const char * rhs)
        {
            m_string.assign(rhs);
            return *this;
        }

        bool SophosString::operator==(const SophosString& rhs) const
        {
            return (m_string == rhs.str());
        }

        bool SophosString::operator!=(const SophosString& rhs) const
        {
            return !(m_string == rhs.str());
        }

        std::string SophosString::str() const
        {
            return m_string;
        }

        std::ostream& operator<<(std::ostream& os, const SophosString& rhs)
        {
            os <<  rhs.str();
            return os;
        }

        void SophosString::limitStringSize()
        {
            uint limit = 5 * 1024;
            uint64_t stringSize = m_string.length() * sizeof(char);
            if (stringSize > limit)
            {
                m_string = m_string.substr(0, limit);
            }
        }
    }
}
