/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <iostream>

namespace Common
{
    namespace EventTypes
    {
        class SophosString
        {
        public:

            /**
             * @brief Copy constructor for SophosString given a std::string
             * @param str, string variable that SophosString will copy.
             */
            SophosString(const std::string& str);

            /**
             * @brief Copy constructor for SophosString given a char pointer
             * @param str, char pointer that SophosString will copy.
             */
            SophosString(const char * str);

            /**
             * @brief Copy constructor for SophosString given another instance of SophosString
             * @param str, SophosString instance that will be copied.
             */
            SophosString(const SophosString& str);

            /**
             * @brief Move constructor for SophosString given a std::string
             * @param str, string variable that SophosString will move.
             */
            SophosString(std::string&& str);

            SophosString() = default;
            ~SophosString() = default;

            /**
             * @brief Provides implicit conversation from SophosString to std::string
             * @returns SophosString as std::string
             */
            operator std::string() const;

            /**
             * @brief Assigns std::string to an instance of SophosString
             * @returns A SophosString instance
             */
            SophosString& operator=(const std::string& rhs);

            /**
             * @brief Assigns a char pointer to an instance of SophosString
             * @param rhs, std::string from which to assign.
             * @returns A SophosString instance
             */
            SophosString& operator=(const char * rhs);

            /**
             * @brief Assigns a SophosString to an instance of SophosString
             * @param rhs, char pointer from which to assign.
             * @returns A SophosString instance
             */
            SophosString& operator=(const SophosString& rhs);

            /**
             * @brief Compares another SophosString to this instance of SophosString
             * @param rhs, SophosString instance from which to assign.
             * @returns True if equal, False if otherwise
             */
            bool operator==(const SophosString& rhs) const;

            /**
             * @brief Compares another SophosString to this instance of SophosString
             * @param rhs, SophosString instance to compare.
             * @returns False if equal, True if otherwise
             */
            bool operator!=(const SophosString& rhs) const;

            /**
             * @brief Getting of the std::string version of the SophosString instance
             * @param rhs, SophosString instance to compare.
             * @returns String value of SophosString
             */
            std::string str() const;

            /**
             * @brief Streams an instance of SophosString into the give stream
             * @param os, stream instance
             * @param rhs, SophosString instance to stream.
             * @returns The stream with the added SophosString
             */
            friend std::ostream& operator<<(std::ostream& os, const SophosString& rhs);

        private:
            /**
             * @brief Compares size of string to max and truncates if string is too long
             */
            void limitStringSize();

            std::string m_string;
        };
    }
}


