#!/usr/bin/env python
# Copyright 2017 Sophos Plc. All rights reserved.

from __future__ import absolute_import,print_function,division,unicode_literals

import unittest
import sys
import os
import base64
import random

import PathManager

import mcsrouter.sec_obfuscation
import mcsrouter.sec_obfuscation_password

class NotRandomGenerator(object):
    def randomBytes(self, size):
        return b"A" * size

class RandomGenerator(object):
    def randomBytes(self, size):
        return bytearray(random.getrandbits(8) for _ in xrange(size))

class TestSECObfuscation(unittest.TestCase):
    def testPassword(self):
        password = mcsrouter.sec_obfuscation_password.getPassword()
        #~ print(password,len(password))
        self.assertEqual(password[0],'V')
        self.assertEqual(len(password),48)

    def testDeobfuscation_3DES(self):
        obfuscated = b"BwhG+aIe8X1jsPM4PpvCMUItpmbuWzlZMFk1GbXs5tFHWsM3h28n7ZJT"
        expected = b"Lorem ipsum dolor sit amet"
        deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

    def testDeobfuscation_3DES_short(self):
        obfuscated = b"Bwh/EjOnVfpnj2j2A7ZK9rcX"
        expected = b"a:b"
        deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

    def testDeobfuscation_AES_1(self):
        obfuscated = b"CCDiD8La6HsOmf0vCG0bTL88m2j9IOy34+9PQ9MiIpTNxJ9GcSzxrWDSHnEI1vyFKtM="
        expected = "foo:bar"
        deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

    def testDeobfuscation_AES_2(self):
        obfuscated = b"CCC37KEkq71emFPR3Zwq2N6aliHg8jSHQt8o0kx1CP2o4GWyCpv5sSjLPYFKHEd4Jrw="
        expected = "foo:bar"
        deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

    def testBadAlgorithmId(self):
        obfuscated = base64.b64encode(
            b"\x06"                                 ## Signature (not 0x07)
            b"\x08"                                 ## Salt size
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63\xb0"     ## Salt
            b"\xf3\x38\x3e\x9b\xc2\x31\x42\x2d"     ## Encrypted data
            b"\xa6\x66\xee\x5b\x39\x59\x30\x59"
            b"\x35\x19\xb5\xec\xe6\xd1\x47\x5a"
            b"\xc3\x37\x87\x6f\x27\xed\x92\x53")
        with self.assertRaisesRegexp(mcsrouter.sec_obfuscation.SECObfuscationException,
                                     r"Unknown obfuscation algorithm-id") as cm:
            deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)

    def testBadSaltLength(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x07"                                 ## Salt size (not 0x08)
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63\xb0"     ## Salt
            b"\xf3\x38\x3e\x9b\xc2\x31\x42\x2d"     ## Encrypted data
            b"\xa6\x66\xee\x5b\x39\x59\x30\x59"
            b"\x35\x19\xb5\xec\xe6\xd1\x47\x5a"
            b"\xc3\x37\x87\x6f\x27\xed\x92\x53")
        with self.assertRaisesRegexp(mcsrouter.sec_obfuscation.SECObfuscationException,
                                     r"Incorrect number of salt bytes") as cm:
            deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)

    def testNoSalt(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x08"                                 ## Salt size
        )
        with self.assertRaisesRegexp(mcsrouter.sec_obfuscation.SECObfuscationException,
                                     r"Ciphertext corrupt: short salt") as cm:
            deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)

    def testShortSalt(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x08"                                 ## Salt size
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63"         ## Salt (one byte too short)
        )
        with self.assertRaisesRegexp(mcsrouter.sec_obfuscation.SECObfuscationException,
                                     r"Ciphertext corrupt: short salt") as cm:
            deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)

    def testNoData(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x08"                                 ## Salt size
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63\xb0"     ## Salt
        )
        with self.assertRaisesRegexp(mcsrouter.sec_obfuscation.SECObfuscationException,
                                     r"Ciphertext corrupt: data short") as cm:
            deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)

    def testBadBase64(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x08"                                 ## Salt size
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63\xb0"     ## Salt
            b"\xf3\x38\x3e\x9b\xc2\x31\x42\x2d"     ## Encrypted data
            b"\xa6\x66\xee\x5b\x39\x59\x30\x59"
            b"\x35\x19\xb5\xec\xe6\xd1\x47\x5a"
            b"\xc3\x37\x87\x6f\x27\xed\x92\x53"
        )
        obfuscated = b"*" + obfuscated[1:]
        with self.assertRaisesRegexp(mcsrouter.sec_obfuscation.SECObfuscationException,
                                     r"Invalid Base64 in SECObfuscation") as cm:
            deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)

    def testRoundTrip_AES(self):
        expected = b"Lorem ipsum dolor sit amet"
        obfuscated = mcsrouter.sec_obfuscation.obfuscate(mcsrouter.sec_obfuscation.ALGO_AES256,expected,RandomGenerator())
        deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

    def testProxyPasswordDeofbuscation(self):
        obfuscated = "CCC4Fcz2iNaH44sdmqyLughrajL7svMPTbUZc/Q4c7yAtSrdM03lfO33xI0XKNU4IBY="
        deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)
        expected = "Ch1pm0nk"
        self.assertEqual(deobfuscated,expected)

    def testRoundTrip_DES3(self):
        expected = b"Lorem ipsum dolor sit amet"
        obfuscated = mcsrouter.sec_obfuscation.obfuscate(mcsrouter.sec_obfuscation.ALGO_3DES,expected,RandomGenerator())
        deobfuscated = mcsrouter.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

if __name__ == '__main__':
    if "-q" in sys.argv:
        logging.disable(logging.CRITICAL)
    unittest.main()
