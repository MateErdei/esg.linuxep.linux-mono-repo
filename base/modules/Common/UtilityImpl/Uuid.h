// Copyright 2022, Sophos Limited. All rights reserved.

#pragma once

#include <array>
#include <stdexcept>
#include <string>

namespace Common::UtilityImpl::Uuid
{
    using Bytes = std::array<uint8_t, 16>;

    /**
     * Converts a hexadecimal character to its value as a number.
     */
    constexpr uint8_t HexToNibble(char hex)
    {
        if (hex >= 'a' && hex <= 'f')
        {
            return hex - 'a' + 10;
        }
        else if (hex >= 'A' && hex <= 'F')
        {
            return hex - 'A' + 10;
        }
        else if (hex >= '0' && hex <= '9')
        {
            return hex - '0';
        }
        else
        {
            throw std::invalid_argument("'" + std::to_string(hex) + "' is not a hexadecimal character");
        }
    }

    static_assert(HexToNibble('0') == 0x0);
    static_assert(HexToNibble('1') == 0x1);
    static_assert(HexToNibble('2') == 0x2);
    static_assert(HexToNibble('3') == 0x3);
    static_assert(HexToNibble('4') == 0x4);
    static_assert(HexToNibble('5') == 0x5);
    static_assert(HexToNibble('6') == 0x6);
    static_assert(HexToNibble('7') == 0x7);
    static_assert(HexToNibble('8') == 0x8);
    static_assert(HexToNibble('9') == 0x9);
    static_assert(HexToNibble('a') == 0xa);
    static_assert(HexToNibble('b') == 0xb);
    static_assert(HexToNibble('c') == 0xc);
    static_assert(HexToNibble('d') == 0xd);
    static_assert(HexToNibble('e') == 0xe);
    static_assert(HexToNibble('f') == 0xf);
    static_assert(HexToNibble('A') == 0xa);
    static_assert(HexToNibble('B') == 0xb);
    static_assert(HexToNibble('C') == 0xc);
    static_assert(HexToNibble('D') == 0xd);
    static_assert(HexToNibble('E') == 0xe);
    static_assert(HexToNibble('F') == 0xf);

    /**
     * Converts a nibble (4 bits) to its representation as a hexadecimal character
     */
    constexpr char NibbleToHex(uint8_t nibble)
    {
        if (nibble <= 9)
        {
            return static_cast<char>('0' + nibble);
        }
        else if (nibble <= 15)
        {
            return static_cast<char>('a' + nibble - 10);
        }
        else
        {
            throw std::invalid_argument("'" + std::to_string(nibble) + "' is not a 4 bit number");
        }
    }

    static_assert(NibbleToHex(0x0) == '0');
    static_assert(NibbleToHex(0x1) == '1');
    static_assert(NibbleToHex(0x2) == '2');
    static_assert(NibbleToHex(0x3) == '3');
    static_assert(NibbleToHex(0x4) == '4');
    static_assert(NibbleToHex(0x5) == '5');
    static_assert(NibbleToHex(0x6) == '6');
    static_assert(NibbleToHex(0x7) == '7');
    static_assert(NibbleToHex(0x8) == '8');
    static_assert(NibbleToHex(0x9) == '9');
    static_assert(NibbleToHex(0xa) == 'a');
    static_assert(NibbleToHex(0xb) == 'b');
    static_assert(NibbleToHex(0xc) == 'c');
    static_assert(NibbleToHex(0xd) == 'd');
    static_assert(NibbleToHex(0xe) == 'e');
    static_assert(NibbleToHex(0xf) == 'f');

    bool IsValid(const std::string& uuidString) noexcept;

    /**
     * Creates a version 5 UUID as defined by RFC 4122.
     * It creates it from a SHA-1 of a namespace (which is a UUID) combined with a name (a string).
     * There are different namespaces for URLs, domains etc.
     * The namespace ensures that the same name under a different namespace would give a different UUID.
     *
     * @param ns The namespace UUID as bytes
     * @param name The name string
     * @return UUID string
     */
    std::string CreateVersion5(const Bytes& ns, const std::string& name);
} // namespace Common::UtilityImpl::Uuid
