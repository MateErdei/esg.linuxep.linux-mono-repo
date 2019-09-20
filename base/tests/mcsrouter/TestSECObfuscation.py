#!/usr/bin/env python3
# Copyright 2017-2019 Sophos Plc. All rights reserved.



import base64
import random
import sys
import unittest

import PathManager

import mcsrouter.utils.sec_obfuscation
import mcsrouter.utils.sec_obfuscation_password


class NotRandomGenerator(object):
    def randomBytes(self, size):
        return b"A" * size

class RandomGenerator(object):
    def random_bytes(self, size):
        return bytearray(random.getrandbits(8) for _ in range(size))

class TestSECObfuscation(unittest.TestCase):
    def testPassword(self):
        password = mcsrouter.utils.sec_obfuscation_password.get_password()
        #~ print(password,len(password))
        self.assertEqual(password[0],ord('V'))
        self.assertEqual(len(password),48)

    def testDeobfuscation_AES_1(self):
        obfuscated = b"CCDiD8La6HsOmf0vCG0bTL88m2j9IOy34+9PQ9MiIpTNxJ9GcSzxrWDSHnEI1vyFKtM="
        expected = "foo:bar"
        deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

    def testDeobfuscation_AES_2(self):
        obfuscated = b"CCC37KEkq71emFPR3Zwq2N6aliHg8jSHQt8o0kx1CP2o4GWyCpv5sSjLPYFKHEd4Jrw="
        expected = "foo:bar"
        deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)
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
        with self.assertRaisesRegex(mcsrouter.utils.sec_obfuscation.SECObfuscationException,
                                     r"Unknown obfuscation algorithm-id") as cm:
            deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)

    def testBadSaltLength(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x07"                                 ## Salt size (not 0x08)
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63\xb0"     ## Salt
            b"\xf3\x38\x3e\x9b\xc2\x31\x42\x2d"     ## Encrypted data
            b"\xa6\x66\xee\x5b\x39\x59\x30\x59"
            b"\x35\x19\xb5\xec\xe6\xd1\x47\x5a"
            b"\xc3\x37\x87\x6f\x27\xed\x92\x53")
        with self.assertRaisesRegex(mcsrouter.utils.sec_obfuscation.SECObfuscationException,
                                     r"Incorrect number of salt bytes") as cm:
            deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)

    def testNoSalt(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x08"                                 ## Salt size
        )
        with self.assertRaisesRegex(mcsrouter.utils.sec_obfuscation.SECObfuscationException,
                                     r"Ciphertext corrupt: short salt") as cm:
            deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)

    def testShortSalt(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x08"                                 ## Salt size
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63"         ## Salt (one byte too short)
        )
        with self.assertRaisesRegex(mcsrouter.utils.sec_obfuscation.SECObfuscationException,
                                     r"Ciphertext corrupt: short salt") as cm:
            deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)

    def testNoData(self):
        obfuscated = base64.b64encode(
            b"\x07"                                 ## Signature
            b"\x08"                                 ## Salt size
            b"\x46\xf9\xa2\x1e\xf1\x7d\x63\xb0"     ## Salt
        )
        with self.assertRaisesRegex(mcsrouter.utils.sec_obfuscation.SECObfuscationException,
                                     r"Ciphertext corrupt: data short") as cm:
            deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)

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
        with self.assertRaisesRegex(mcsrouter.utils.sec_obfuscation.SECObfuscationException,
                                     r"Invalid Base64 in SECObfuscation") as cm:
            deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)

    def testRoundTrip_AES(self):
        expected = "Lorem ipsum dolor sit amet"
        input_for_obfuscate = expected.encode('ascii')
        obfuscated = mcsrouter.utils.sec_obfuscation.obfuscate(mcsrouter.utils.sec_obfuscation.ALGO_AES256,input_for_obfuscate,RandomGenerator())
        deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)
        self.assertEqual(deobfuscated,expected)

    def testProxyPasswordDeofbuscation(self):
        obfuscated = "CCCj7sOF/IMdsPr1YxSIC0XjQcBmqy4kRtg7wwV0uCFxwzGl2qNaqk4lYs/6cQmFNLY="
        deobfuscated = mcsrouter.utils.sec_obfuscation.deobfuscate(obfuscated)
        expected = "password"
        self.assertEqual(deobfuscated,expected)

if __name__ == '__main__':
    if "-q" in sys.argv:
        logging.disable(logging.CRITICAL)
    unittest.main()
