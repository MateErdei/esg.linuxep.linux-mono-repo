

#include <iostream>
#include <string.h>
#include <vector>

namespace Obscurity
{
    static const std::vector<unsigned char> CKeyDo_data{ 0x10, 0xd4, 0xfa, 0x82, 0xa0, 0x3a,
                                                         0x67, 0x90, 0xbe, 0x7d, 0x26, 0x2f };

    class CKeyDo
    {
    public:
        static std::vector<unsigned char> GetData() { return CKeyDo_data; }
    };

} // namespace Obscurity

namespace Obscurity
{
    static const std::vector<unsigned char> CKeyFa_data{ 0xa6, 0xb2, 0x17, 0x93, 0xc0, 0xf8,
                                                         0x8e, 0x7a, 0x62, 0xef, 0xe8, 0x1e };

    class CKeyFa
    {
    public:
        static std::vector<unsigned char> GetData() { return CKeyFa_data; }
    };

} // namespace Obscurity

namespace Obscurity
{
    static const std::vector<unsigned char> CKeyMi_data{ 0x48, 0x48, 0x48, 0xc6, 0xc4, 0x78, 0x0,  0x14,
                                                         0xb3, 0xbf, 0x9f, 0xfc, 0xce, 0x34, 0x40, 0x2c,
                                                         0x8,  0x10, 0x90, 0x5f, 0xfd, 0x1f, 0x5b, 0xbf };

    class CKeyMi
    {
    public:
        static std::vector<unsigned char> GetData() { return CKeyMi_data; }
    };

} // namespace Obscurity

namespace Obscurity
{
    static const std::vector<unsigned char> CKeyRa_data{ 0xae, 0xae, 0xae, 0x9b, 0x99, 0xf1, 0xd,  0x19,
                                                         0x50, 0x5c, 0x7c, 0x1c, 0x2e, 0x2,  0x1a, 0x76,
                                                         0xc1, 0xd9, 0x59, 0x20, 0x82, 0x57, 0x10, 0xf4 };

    class CKeyRa
    {
    public:
        static std::vector<unsigned char> GetData() { return CKeyRa_data; }
    };

} // namespace Obscurity

#define SecureBuffer std::vector
using namespace Obscurity;

static std::vector<unsigned char> R1(const std::vector<unsigned char>& data)
{
    std::vector<unsigned char> ret(data.size());

    // Reverse Transposition of data - ADCB becomes ABCD
    for (unsigned int i = 0; i < data.size(); i++)
    {
        unsigned int mod2 = i % 2;
        if (0 == mod2)
            ret[i] = data[i / 2]; // even
        else
            ret[i] = data[(data.size() - (i / 2)) - 1]; // odd
    }
    return ret;
}

static unsigned int getIndex(unsigned int salt)
{
    static const unsigned char KEY[] = "FDGASSkwpodkfgfspwoegre;[addq[pad.col";
    unsigned int mod3 = salt % 3;
    if (0 == mod3)
    {
        return salt * 13 % sizeof(KEY);
    }
    else if (1 == mod3)
    {
        return salt * 11 % sizeof(KEY);
    }
    else
    {
        return salt * 7 % sizeof(KEY);
    }
}

static unsigned char GetMask(unsigned int salt)
{
    static const unsigned char KEY[] = "FDGASSkwpodkfgfspwoegre;[addq[pad.col";
    unsigned char mask = 0;
    unsigned int mod3 = salt % 3;
    if (0 == mod3)
    {
        mask = KEY[salt * 13 % sizeof(KEY)];
    }
    else if (1 == mod3)
    {
        mask = KEY[salt * 11 % sizeof(KEY)];
    }
    else
    {
        mask = KEY[salt * 7 % sizeof(KEY)];
    }
    return mask;
}

static std::vector<unsigned char> GetMasks(const std::vector<unsigned char>& data)
{
    std::vector<unsigned char> ret(data.size());
    for (unsigned int i = 0; i < data.size(); i++)
    {
        ret[i] = GetMask(i);
    }
    return ret;
}

static std::vector<unsigned char> GetIndexes(const std::vector<unsigned char>& data)
{
    std::vector<unsigned char> ret(data.size());
    for (unsigned int i = 0; i < data.size(); i++)
    {
        ret[i] = getIndex(i);
    }
    return ret;
}

static std::vector<unsigned char> R2(const std::vector<unsigned char>& data)
{
    // Reverse O2()
    std::vector<unsigned char> ret(data.size());
    for (unsigned int i = 0; i < data.size(); i++)
    {
        ret[i] = data[i] ^ GetMask(i);
    }
    return ret;
}

static std::vector<unsigned char> R3(const std::vector<unsigned char>& data)
{
    // Reverse data transposition
    std::vector<unsigned char> ret((data.size() / 2) + 4);
    unsigned int tmpIndex = 0;
    for (unsigned int i = 0; i < data.size(); i++)
    {
        unsigned int mod8 = i % 8;
        if (0 == mod8)
            ret[tmpIndex++] = data[i];
        else if (3 == mod8)
            ret[tmpIndex++] = data[i];
        else if (5 == mod8)
            ret[tmpIndex++] = data[i];
        else if (6 == mod8)
            ret[tmpIndex++] = data[i];
    }
    ret.resize(tmpIndex);
    return ret;
}

static void memcpy_s(void* dest, size_t destSize, void* src, size_t srcSize)
{
    memcpy(dest, src, srcSize);
}

static SecureBuffer<char> GetPassword()
{
    SecureBuffer<unsigned char> sect1, sect2, sect3, sect4;
    SecureBuffer<unsigned char> data1;

    // Unobscure the sections of the password
    data1 = R1(CKeyDo::GetData());
    sect1 = R2(data1);

    data1 = R3(CKeyRa::GetData());
    sect2 = R2(data1);

    data1 = R3(CKeyMi::GetData());
    sect3 = R1(data1);

    data1 = R2(CKeyFa::GetData());
    sect4 = R1(data1);

    SecureBuffer<char> ret(sect1.size() + sect2.size() + sect3.size() + sect4.size());
    size_t offset = 0;
    memcpy_s(&ret[offset], ret.size() - offset, &sect1[0], sect1.size());
    offset += sect1.size();
    memcpy_s(&ret[offset], ret.size() - offset, &sect2[0], sect2.size());
    offset += sect2.size();
    memcpy_s(&ret[offset], ret.size() - offset, &sect3[0], sect3.size());
    offset += sect3.size();
    memcpy_s(&ret[offset], ret.size() - offset, &sect4[0], sect4.size());
    //~ offset += sect4.size();

    return ret;
}

static void dump(const std::string& name, const SecureBuffer<char>& data)
{
    std::cerr << std::endl << name << std::endl;
    for (auto c : data)
    {
        unsigned char x = (unsigned char)c;
        std::cerr << (int)x << "\t" << c << std::endl;
    }
}

static void dump(const std::string& name, const SecureBuffer<unsigned char>& data)
{
    std::cerr << std::endl << name << std::endl;
    for (auto c : data)
    {
        unsigned char x = (unsigned char)c;
        std::cerr << (int)x << "\t" << c << std::endl;
    }
}

int main()
{
    SecureBuffer<char> password = GetPassword();
    dump("password", password);

    //~ SecureBuffer<unsigned char> sect1, sect2, sect3, sect4;
    //~ SecureBuffer<unsigned char> data1;

    //~ data1 = R1(CKeyDo::GetData());
    //~ dump("R1",data1);

    //~ data1 = R2(CKeyFa::GetData());
    //~ dump("R2",data1);

    //~ data1 = GetMasks(CKeyFa::GetData());
    //~ dump("GetMasks",data1);
    //~ data1 = GetIndexes(CKeyFa::GetData());
    //~ dump("GetIndexes",data1);

    //~ data1 = R3(CKeyRa::GetData());
    //~ dump("R3",data1);

    //~ std::cerr << "getIndex " << getIndex(4) << std::endl;

    return 0;
}
