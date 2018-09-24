#!/usr/bin/env python
# Copyright 2017 Sophos Plc. All rights reserved.

from __future__ import absolute_import,print_function,division,unicode_literals

## From //wincomms/latest-dev/mcs-endpoint/product/src/McsClient/SECObfuscation/...

## From key headers
C_KEY_DO = [ 0x10, 0xd4, 0xfa, 0x82, 0xa0, 0x3a, 0x67, 0x90, 0xbe, 0x7d, 0x26, 0x2f ]
C_KEY_RA = [ 0xae, 0xae, 0xae, 0x9b, 0x99, 0xf1, 0xd, 0x19, 0x50, 0x5c, 0x7c, 0x1c, 0x2e, 0x2
    , 0x1a, 0x76, 0xc1, 0xd9, 0x59, 0x20, 0x82, 0x57, 0x10, 0xf4 ]
C_KEY_MI = [ 0x48, 0x48, 0x48, 0xc6, 0xc4, 0x78, 0x0, 0x14, 0xb3, 0xbf, 0x9f, 0xfc, 0xce,
             0x34, 0x40, 0x2c, 0x8, 0x10, 0x90, 0x5f, 0xfd, 0x1f, 0x5b, 0xbf ]
C_KEY_FA = [ 0xa6, 0xb2, 0x17, 0x93, 0xc0, 0xf8, 0x8e, 0x7a, 0x62, 0xef, 0xe8, 0x1e ]


## From ObfuscationImpl.cpp

#~ static std::vector<unsigned char> R1(const std::vector<unsigned char>& data)
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

def R1(inputArray):
    outputArray = inputArray[:]
    dataSize = len(outputArray)
    ## Reverse Transposition of data - ADCB becomes ABCD
    for i in xrange(dataSize):
        mod2 = i % 2
        if mod2 == 0:
            outputArray[i] = inputArray[i // 2] ## even
        else:
            outputArray[i] = inputArray[dataSize - i // 2 - 1]
    return outputArray



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

def getIndex(salt):
    KEY = b"FDGASSkwpodkfgfspwoegre;[addq[pad.col\x00"
    lenKey = len(KEY)
    mod3 = salt % 3
    if mod3 == 0:
        return (salt * 13) % lenKey
    elif mod3 == 1:
        return (salt * 11) % lenKey
    else:
        return (salt * 7) % lenKey


def getMask(salt):
    KEY = b"FDGASSkwpodkfgfspwoegre;[addq[pad.col\x00"
    lenKey = len(KEY)
    mod3 = salt % 3
    if mod3 == 0:
        return ord(KEY[(salt * 13) % lenKey])
    elif mod3 == 1:
        return ord(KEY[(salt * 11) % lenKey])
    else:
        return ord(KEY[(salt * 7) % lenKey])


#~ static std::vector<unsigned char> R2(const std::vector<unsigned char>& data)
#~ {
#~ // Reverse O2()
#~ std::vector<unsigned char> ret(data.size());
#~ for (unsigned int i = 0; i < data.size(); i++)
#~ {
#~ ret[i] = data[i] ^ GetMask(i);
#~ }
#~ return ret;
#~ }

def R2(inputArray):
    """
    Reverse O2()?
    """
    outputArray = []
    for (i,c) in enumerate(inputArray):
        outputArray.append(c ^ getMask(i))

    return outputArray

def getMasks(inputArray):
    outputArray = []
    for (i,c) in enumerate(inputArray):
        outputArray.append(getMask(i))

    return outputArray

def getIndexes(inputArray):
    outputArray = []
    for (i,c) in enumerate(inputArray):
        outputArray.append(getIndex(i))

    return outputArray

#~ static std::vector<unsigned char> R3(const std::vector<unsigned char>& data)
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

def R3(inputArray):
    """
    Reverse data transposition
    """
    outputArray = []
    for i,c in enumerate(inputArray):
        mod8 = i % 8
        if mod8 in (0,3,5,6):
            outputArray.append(c)

    return outputArray


#~ static SecureBuffer<char> GetPassword()
#~ {
#~ SecureBuffer<unsigned char> sect1, sect2, sect3, sect4;
#~ SecureBuffer<unsigned char> data1;

#~ // Unobscure the sections of the password
#~ data1 = R1(CKeyDo::GetData());
#~ sect1 = R2(data1);

#~ data1 = R3(CKeyRa::GetData());
#~ sect2 = R2(data1);

#~ data1 = R3(CKeyMi::GetData());
#~ sect3 = R1(data1);

#~ data1 = R2(CKeyFa::GetData());
#~ sect4 = R1(data1);

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

def getPasswordUncached():
    data = R1(C_KEY_DO)
    sect1 = R2(data)

    data = R3(C_KEY_RA)
    sect2 = R2(data)

    data = R3(C_KEY_MI)
    sect3 = R1(data)

    data = R2(C_KEY_FA)
    sect4 = R1(data)

    ret = []
    for s in (sect1,sect2,sect3,sect4):
        for c in s:
            ret.append(chr(c))

    return b"".join(ret)

GL_PASSWORD = None

def getPassword():
    global GL_PASSWORD
    if GL_PASSWORD is None:
        GL_PASSWORD = getPasswordUncached()
    return GL_PASSWORD

def dump(name, array):
    print("\n%s"%name)
    for c in array:
        if isinstance(c,str):
            print(ord(c),c)
        else:
            print(c,chr(c))

if __name__ == '__main__':
    password = getPassword()
    dump("password",password)

    #~ data = R1(C_KEY_DO)
    #~ dump("R1",data)

    #~ data = R2(C_KEY_FA)
    #~ dump("R2",data)
    #~ data = getMasks(C_KEY_FA)
    #~ dump("getMasks",data)
    #~ data = getIndexes(C_KEY_FA)
    #~ dump("getIndexes",data)

    #~ data = R3(C_KEY_RA)
    #~ dump("R3",data)

    #~ print("getIndex",getIndex(4))
