/******************************************************************************************************

Copyright 2018-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Base64.h"

#include "Logger.h"

#include "Common/Obfuscation/IBase64Exception.h"

#include <algorithm>
#include <vector>
#include <openssl/evp.h>
namespace
{
    using namespace Common::ObfuscationImpl;

    unsigned encode_buffer_size( size_t original_size )
    {
        return ( ( unsigned( original_size ) + 2 ) / 3 ) * 4;
    }
    /**
     * The set of base64 characters.
     */
    const std::string sBase64Chars("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789"
                                   "+/");

    /**
     *  Get the next character for decoding from a base64 string.
     * @param it
     * @param itEnd
     * @return
     */
    unsigned int GetEncodedCharacter(std::string::const_iterator& it, const std::string::const_iterator& itEnd)
    {
        // There should always be sufficient characters, so if we run out, the supplied encoded
        // string is invalid.

        if (it == itEnd)
        {
            LOGDEBUG("Too few characters in encoded string.");
            throw Common::Obfuscation::IBase64Exception("SECDeobfuscation Failed.");
        }

        // Get the next encoded character, and look it up in the string of allowed characters. If it
        // is not found, the supplied encoded string is not valid. An array of characters, indexed
        // by the binary value of the base64 character, would be more efficient here, but we are
        // generally concerned only with very short strings.

        char ch = (*it++);
        size_t pos = sBase64Chars.find(ch);

        if (pos == std::string::npos)
        {
            LOGDEBUG("Invalid character in encoded string.");
            throw Common::Obfuscation::IBase64Exception("SECDeobfuscation Failed.");
        }

        // Return the binary value of the character.

        return static_cast<unsigned int>(pos);
    }

    void erase_all(std::string& s, const std::string& valueToErase)
    {
        for (char charToRemove : valueToErase)
        {
            s.erase(std::remove(s.begin(), s.end(), charToRemove), s.end());
        }
    }

    void erase_from_end(std::string& s, char valueToErase)
    {
        while (!s.empty() && s.back() == valueToErase)
        {
            s.erase(s.begin() + s.size() - 1, s.end());
        }
    }
} // namespace

namespace Common
{
    namespace ObfuscationImpl
    {
        std::string Base64::Encode(const std::string& orig)
        {
            if (orig.size() == 0)
                return "";

            unsigned buff_size = encode_buffer_size( orig.size() );
            std::vector<unsigned char> b64_buff( buff_size + 1 );
            EVP_EncodeBlock( &*b64_buff.begin(), reinterpret_cast<const unsigned char*>(&*orig.begin()), int( orig.size() ) );

            return std::string( b64_buff.begin(), b64_buff.begin() + buff_size );
        }
        /**
         * Decode a base64 string.
         * @param sEncoded
         * @return
         */
        std::string Base64::Decode(const std::string& sEncoded)
        {
            // Take a copy of the encoded string, and remove any newline and padding characters. There
            // should only be at most two padding characters.

            std::string s = sEncoded;
            std::string sPlain;
            erase_all(s, "\r\n");

            if (s.empty())
            {
                return sPlain;
            }

            erase_from_end(s, '=');

            // If the encoded string is empty, the decoded string is also empty.
            if (s.empty())
            {
                return sPlain;
            }

            // Decode the string. Each four input characters result in up to three output characters. The
            // value of an output character can be made up from bits of the current and previous
            // input characters.

            unsigned int uPrev = 0, uNext = 0;

            for (std::string::const_iterator it = s.begin(), itEnd = s.end(); it != itEnd;)
            {
                uPrev = GetEncodedCharacter(it, itEnd);
                uNext = GetEncodedCharacter(it, itEnd);

                sPlain += static_cast<char>((uPrev << 2) + (uNext >> 4));

                if (it == itEnd)
                {
                    break;
                }

                uPrev = uNext;
                uNext = GetEncodedCharacter(it, itEnd);

                sPlain += static_cast<char>(((uPrev & 0x1f) << 4) + (uNext >> 2));

                if (it == itEnd)
                {
                    break;
                }

                uPrev = uNext;
                uNext = GetEncodedCharacter(it, itEnd);

                sPlain += static_cast<char>(((uPrev & 0x03) << 6) + uNext);
            }

            // Return the decoded string.

            return sPlain;
        }
    } // namespace ObfuscationImpl
} // namespace Common
