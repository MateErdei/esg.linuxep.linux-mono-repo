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
             * @returns const ref String of SophosString
             */
            const std::string& str() const;

            /**
             * @brief Gets size of SophosString as equivalent to std::string method
             * @returns size_t value for size of SophosString
             */
            size_t size() const;

            /**
             * @brief Gets length of SophosString as equivalent to std::string method
             * @returns size_t value for length of SophosString
             */
            size_t length() const;

            /**
             * @brief Searches the string for the last occurrence of the sequence specified by its arguments.
             * @param str, another string with the subject to search for.
             * @param pos, Position of the last character in the string to be considered as the beginning of a match.
             * Any value greater or equal than the string length (including string::npos) means that the entire string is searched.
             * Note: The first character is denoted by a value of 0 (not 1).
             * @returns size_t, position of the first character of the last match.
             * If no matches were found, the function returns string::npos.
             */
            size_t rfind(const std::string& str, size_t pos = std::string::npos) const noexcept;

            /**
             * @brief Searches the string for the last occurrence of the sequence specified by its arguments.
             * @param s, pointer to an array of characters.
             * If argument n is specified (3), the sequence to match are the first n characters in the array.
             * Otherwise (2), a null-terminated sequence is expected: the length of the sequence to match is determined by the first occurrence of a null character.
             * @param pos, Position of the last character in the string to be considered as the beginning of a match.
             * Any value greater or equal than the string length (including string::npos) means that the entire string is searched.
             * Note: The first character is denoted by a value of 0 (not 1).
             * @returns size_t, position of the first character of the last match.
             * If no matches were found, the function returns string::npos.
             */
            size_t rfind(const char* s, size_t pos = std::string::npos) const;

            /**
             * @brief Searches the string for the last occurrence of the sequence specified by its arguments.
             * @param s, pointer to an array of characters.
             * If argument n is specified (3), the sequence to match are the first n characters in the array.
             * Otherwise (2), a null-terminated sequence is expected: the length of the sequence to match is determined by the first occurrence of a null character.
             * @param pos, position of the last character in the string to be considered as the beginning of a match.
             * Any value greater or equal than the string length (including string::npos) means that the entire string is searched.
             * Note: The first character is denoted by a value of 0 (not 1).
             * @param n, length of sequence of characters to match.
             * @returns size_t, position of the first character of the last match.
             * If no matches were found, the function returns string::npos.
             */
            size_t rfind(const char* s, size_t pos, size_t n) const;

            /**
             * @brief Searches the string for the last occurrence of the sequence specified by its arguments.
             * @param c, individual character to be searched for.
             * @param pos, Position of the last character in the string to be considered as the beginning of a match.
             * Any value greater or equal than the string length (including string::npos) means that the entire string is searched.
             * Note: The first character is denoted by a value of 0 (not 1).
             * @returns size_t, position of the first character of the last match.
             * If no matches were found, the function returns string::npos.
             */
            size_t rfind(char c, size_t pos = std::string::npos) const noexcept;

            /**
             * @brief Returns a newly constructed string object with its value initialized to a copy of a substring of this object.
             * @param pos, position of the first character to be copied as a substring.
             * If this is equal to the string length, the function returns an empty string.
             * If this is greater than the string length, it throws out_of_range.
             * Note: The first character is denoted by a value of 0 (not 1).
             * @param len, number of characters to include in the substring (if the string is shorter, as many characters as possible are used).
             * A value of string::npos indicates all characters until the end of the string.
             * @return std::string, a string object with a substring of this object.
             */
            std::string substr(size_t pos = 0, size_t len = std::string::npos) const;

            /**
             * @brief Returns a pointer to an array that contains a null-terminated sequence of characters (i.e., a C-string) representing the current value of the string object.
             * @return const char *, a pointer to the c-string representation of the string object's value.
             */
            const char * data() const;

            /**
             * @brief Returns a reference to the last character of the string.
             * @return const char &, a reference to the last character in the string.
             */
            const char & back() const;

            /**
             * @brief Returns a reference to the last character of the string.
             * @return char &, a reference to the last character in the string.
             */
            char & back();

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


