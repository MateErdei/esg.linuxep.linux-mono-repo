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

        const std::string& SophosString::str() const
        {
            return m_string;
        }

        size_t SophosString::size() const
        {
            return m_string.size();
        }

        size_t SophosString::length() const
        {
            return m_string.length();
        }

        size_t SophosString::rfind(const std::string& str, size_t pos) const noexcept
        {
            return m_string.rfind(str, pos);
        }

        size_t SophosString::rfind(const char* s, size_t pos) const
        {
            return m_string.rfind(s, pos);
        }

        size_t SophosString::rfind(const char* s, size_t pos, size_t n) const
        {
            return m_string.rfind(s, pos, n);
        }

        size_t SophosString::rfind(char c, size_t pos) const noexcept
        {
            return m_string.rfind(c, pos);
        }

        std::string SophosString::substr(size_t pos, size_t len) const
        {
            return m_string.substr(pos, len);
        }

        const char * SophosString::data() const
        {
            return m_string.data();
        }

        const char& SophosString::back() const
        {
            return m_string.back();
        }

        char& SophosString::back()
        {
            return m_string.back();
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
