#!/usr/bin/env python
# Copyright 2017 Sophos Plc. All rights reserved.
"""
sec_obfuscation_password Module
"""

#pylint: disable=no-self-use



# From
# //wincomms/latest-dev/mcs-endpoint/product/src/McsClient/SECObfuscation/...

# From key headers
C_KEY_DO = [
    0x10,
    0xd4,
    0xfa,
    0x82,
    0xa0,
    0x3a,
    0x67,
    0x90,
    0xbe,
    0x7d,
    0x26,
    0x2f]
C_KEY_RA = [
    0xae,
    0xae,
    0xae,
    0x9b,
    0x99,
    0xf1,
    0xd,
    0x19,
    0x50,
    0x5c,
    0x7c,
    0x1c,
    0x2e,
    0x2,
    0x1a,
    0x76,
    0xc1,
    0xd9,
    0x59,
    0x20,
    0x82,
    0x57,
    0x10,
    0xf4]
C_KEY_MI = [
    0x48,
    0x48,
    0x48,
    0xc6,
    0xc4,
    0x78,
    0x0,
    0x14,
    0xb3,
    0xbf,
    0x9f,
    0xfc,
    0xce,
    0x34,
    0x40,
    0x2c,
    0x8,
    0x10,
    0x90,
    0x5f,
    0xfd,
    0x1f,
    0x5b,
    0xbf]
C_KEY_FA = [
    0xa6,
    0xb2,
    0x17,
    0x93,
    0xc0,
    0xf8,
    0x8e,
    0x7a,
    0x62,
    0xef,
    0xe8,
    0x1e]


# From ObfuscationImpl.cpp

#~ static std::vector<unsigned char> reverse_1(const std::vector<unsigned char>& data)
#~ {
#~ std::vector<unsigned char> ret(data.size());

#~ // Reverse Transposition of data - ADCB becomes ABCD
#~ for (unsigned int i = 0; i < data.size(); i++)
#~ {
#~ unsigned int mod2 = i % 2;
#~ if (0 == mod2)
#~ ret[i] = data[i / 2]; //even
#~ else
#~ ret[i] = data[(data.size() - (i / 2)) - 1]; //odd
#~ }
#~ return ret;
#~ }

def reverse_1(input_array):
    """
    reverse_1
    """
    output_array = input_array[:]
    data_size = len(output_array)
    # Reverse Transposition of data - ADCB becomes ABCD
    for i in range(data_size):
        mod2 = i % 2
        if mod2 == 0:
            output_array[i] = input_array[i // 2]  # even
        else:
            output_array[i] = input_array[data_size - i // 2 - 1]
    return output_array


#~ static unsigned char GetMask(unsigned int salt)
#~ {
#~ static const unsigned char KEY[] = "FDGASSkwpodkfgfspwoegre;[addq[pad.col";
#~ unsigned char mask = 0;
#~ unsigned int mod3 = salt % 3;
#~ if (0 == mod3)
#~ {
#~ mask = KEY[salt * 13 % sizeof(KEY)];
#~ }
#~ else if (1 == mod3)
#~ {
#~ mask = KEY[salt * 11 % sizeof(KEY)];
#~ }
#~ else
#~ {
#~ mask = KEY[salt * 7 % sizeof(KEY)];
#~ }
#~ return mask;
#~ }

def get_index(salt):
    """
    get_index
    """
    key = "FDGASSkwpodkfgfspwoegre;[addq[pad.col\x00"
    key_length = len(key)
    mod3 = salt % 3
    if mod3 == 0:
        return (salt * 13) % key_length
    elif mod3 == 1:
        return (salt * 11) % key_length
    return (salt * 7) % key_length


def get_mask(salt):
    """
    get_mask
    """
    key = "FDGASSkwpodkfgfspwoegre;[addq[pad.col\x00"
    key_length = len(key)
    mod3 = salt % 3
    if mod3 == 0:
        return ord(key[(salt * 13) % key_length])
    elif mod3 == 1:
        return ord(key[(salt * 11) % key_length])
    return ord(key[(salt * 7) % key_length])


#~ static std::vector<unsigned char> reverse_2(const std::vector<unsigned char>& data)
#~ {
#~ // Reverse O2()
#~ std::vector<unsigned char> ret(data.size());
#~ for (unsigned int i = 0; i < data.size(); i++)
#~ {
#~ ret[i] = data[i] ^ GetMask(i);
#~ }
#~ return ret;
#~ }

def reverse_2(input_array):
    """
    Reverse O2()?
    """
    #pylint: disable=unused-variable
    output_array = []
    for (index, value) in enumerate(input_array):
        output_array.append(value ^ get_mask(index))

    return output_array


def get_masks(input_array):
    """
    get_masks
    """
    #pylint: disable=unused-variable
    output_array = []
    for (index, value) in enumerate(input_array):
        output_array.append(get_mask(index))

    return output_array


def get_indexes(input_array):
    """
    get_indexes
    """
    #pylint: disable=unused-variable
    output_array = []
    for (index, value) in enumerate(input_array):
        output_array.append(get_index(index))

    return output_array

#~ static std::vector<unsigned char> reverse_3(const std::vector<unsigned char>& data)
#~ {
#~ // Reverse data transposition
#~ std::vector<unsigned char> ret((data.size() / 2) + 4);
#~ unsigned int tmpIndex = 0;
#~ for (unsigned int i = 0; i < data.size(); i++)
#~ {
#~ unsigned int mod8 = i % 8;
#~ if (0 == mod8)
#~ ret[tmpIndex++] = data[i];
#~ else if (3 == mod8)
#~ ret[tmpIndex++] = data[i];
#~ else if (5 == mod8)
#~ ret[tmpIndex++] = data[i];
#~ else if (6 == mod8)
#~ ret[tmpIndex++] = data[i];
#~ }
#~ ret.resize(tmpIndex);
#~ return ret;
#~ }


def reverse_3(input_array):
    """
    Reverse data transposition
    """
    output_array = []
    for index, value in enumerate(input_array):
        mod8 = index % 8
        if mod8 in (0, 3, 5, 6):
            output_array.append(value)

    return output_array


#~ static SecureBuffer<char> GetPassword()
#~ {
#~ SecureBuffer<unsigned char> sect1, sect2, sect3, sect4;
#~ SecureBuffer<unsigned char> data1;

#~ // Unobscure the sections of the password
#~ data1 = reverse_1(CKeyDo::GetData());
#~ sect1 = reverse_2(data1);

#~ data1 = reverse_3(CKeyRa::GetData());
#~ sect2 = reverse_2(data1);

#~ data1 = reverse_3(CKeyMi::GetData());
#~ sect3 = reverse_1(data1);

#~ data1 = reverse_2(CKeyFa::GetData());
#~ sect4 = reverse_1(data1);

#~ SecureBuffer<char> ret(sect1.size() + sect2.size() + sect3.size() + sect4.size());
#~ size_t offset = 0;
#~ memcpy_s(&ret[offset], ret.size() - offset, &sect1[0], sect1.size());
#~ offset += sect1.size();
#~ memcpy_s(&ret[offset], ret.size() - offset, &sect2[0], sect2.size());
#~ offset += sect2.size();
#~ memcpy_s(&ret[offset], ret.size() - offset, &sect3[0], sect3.size());
#~ offset += sect3.size();
#~ memcpy_s(&ret[offset], ret.size() - offset, &sect4[0], sect4.size());
#~ offset += sect4.size();

#~ return ret;
#~ }

def get_password_uncached():
    """
    get_password_uncached
    """
    data = reverse_1(C_KEY_DO)
    sect1 = reverse_2(data)

    data = reverse_3(C_KEY_RA)
    sect2 = reverse_2(data)

    data = reverse_3(C_KEY_MI)
    sect3 = reverse_1(data)

    data = reverse_2(C_KEY_FA)
    sect4 = reverse_1(data)

    ret = []
    for sect in (sect1, sect2, sect3, sect4):
        for item in sect:
            ret.append(item)
    return bytes(ret)

GL_PASSWORD = None

def get_password():
    """
    get_password
    """
    # pylint: disable=global-statement
    global GL_PASSWORD
    if GL_PASSWORD is None:
        GL_PASSWORD = get_password_uncached()
    return GL_PASSWORD


def dump(name, array):
    """
    dump
    """
    print("\n%s" % name)
    for item in array:
        if isinstance(item, str):
            print(ord(item), item)
        else:
            print(item, chr(item))


if __name__ == '__main__':
    PASSWORD = get_password()
    dump("password", PASSWORD)

    #~ data = reverse_1(C_KEY_DO)
    #~ dump("reverse_1",data)

    #~ data = reverse_2(C_KEY_FA)
    #~ dump("reverse_2",data)
    #~ data = get_masks(C_KEY_FA)
    #~ dump("get_masks",data)
    #~ data = get_indexes(C_KEY_FA)
    #~ dump("get_indexes",data)

    #~ data = reverse_3(C_KEY_RA)
    #~ dump("reverse_3",data)

    #~ print("get_index",get_index(4))
