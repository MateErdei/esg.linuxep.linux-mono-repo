#!/usr/bin/env python
# Copyright 2017 Sophos Plc. All rights reserved.

from __future__ import absolute_import,print_function,division,unicode_literals

from Crypto.Cipher import DES3
from Crypto.Protocol.KDF import PBKDF2
from Crypto.Hash import SHA512, HMAC

class SECObfuscationException(Exception):
    pass

class SECObfuscation(object):
    def getPassword(self):
        from . import SECObfuscationPassword
        return SECObfuscationPassword.getPassword()

    def removePadding(self, text):
        padding = ord(text[-1])
        if padding > self.BLOCK_LENGTH:
            raise SECObfuscationException("Padding incorrect")

        return text[:-padding]

    def addPadding(self, text):
        padding = self.BLOCK_LENGTH - len(text)%self.BLOCK_LENGTH
        return text + chr(padding) * padding

    def splitKeyIV(self, keyiv):
        key = keyiv[0:self.KEY_LENGTH]
        iv = keyiv[self.KEY_LENGTH:self.KEY_LENGTH+self.IV_LENGTH]

        return (key,iv)

    def deobfuscate(self, salt, cipherText):
        key, iv = self.createSessionKey(salt)
        cipher = self.createCipher(key, iv)
        return self.removePadding(cipher.decrypt(cipherText))

    def obfuscate(self, salt, plainText):
        key, iv = self.createSessionKey(salt)
        cipher = self.createCipher(key, iv)
        return cipher.encrypt(self.addPadding(plainText))


## Algorithm enumeration:
ALGO_3DES = 7
ALGO_AES256 = 8

class ThreeDES(SECObfuscation):
    ALGORITHM_MARKER_BYTE = ALGO_3DES
    SALT_LENGTH = 8
    KEY_LENGTH = 24
    BLOCK_LENGTH = 8
    IV_LENGTH = BLOCK_LENGTH

    def createCipher(self, key, iv):
        return DES3.new(key, DES3.MODE_CBC, IV=iv)

    def createSessionKey(self, salt):
        """
        @return (key,iv) for salt
        """
        import hashlib

        password = self.getPassword()

        keyiv = b""
        previousHash = b""

        while len(keyiv) < self.KEY_LENGTH + self.IV_LENGTH:
            m = hashlib.md5()
            m.update(previousHash)
            m.update(password)
            m.update(salt)
            previousHash = m.digest()
            keyiv += previousHash

        return self.splitKeyIV(keyiv)


class AES256(SECObfuscation):
    ALGORITHM_MARKER_BYTE = ALGO_AES256
    SALT_LENGTH = 32
    KEY_LENGTH = 256 // 8
    BLOCK_LENGTH = 128 // 8
    IV_LENGTH = BLOCK_LENGTH
    KEY_ITERATIONS = 50000

    def createCipher(self, key, iv):
        from Crypto.Cipher import AES
        return AES.new(key, AES.MODE_CBC, IV=iv)

    def createSessionKey(self, salt):
        """
        @return (key,iv) for salt
        """

        password = self.getPassword()

        prf = lambda p,s: HMAC.new(p, s, SHA512).digest()
        keyiv = PBKDF2(password, salt,
                       dkLen=self.KEY_LENGTH + self.IV_LENGTH,
                       count=self.KEY_ITERATIONS,
                       prf=prf
                       )

        return self.splitKeyIV(keyiv)

def getImplementationFromEmbeddedAlgorithmByte(embeddedAlgorithmByte):
    if embeddedAlgorithmByte == ThreeDES.ALGORITHM_MARKER_BYTE:
        return ThreeDES()
    elif embeddedAlgorithmByte == AES256.ALGORITHM_MARKER_BYTE:
        return AES256()

    raise SECObfuscationException("Unknown obfuscation algorithm-id")


def getImplementation(rawObfuscated):
    embeddedAlgorithmByte = ord(rawObfuscated[0])
    return getImplementationFromEmbeddedAlgorithmByte(embeddedAlgorithmByte)

def deobfuscate(base64Obfuscated):
    """
    @param base64Obfuscated BASE64 encoded obfuscated text
    @returns raw plain text
    """
    import base64
    try:
        rawObfuscated = base64.b64decode(base64Obfuscated)
    except TypeError as e:
        raise SECObfuscationException("Invalid Base64 in SECObfuscation")

    impl = getImplementation(rawObfuscated)
    assert impl is not None

    saltLength = ord(rawObfuscated[1])
    if saltLength != impl.SALT_LENGTH:
        raise SECObfuscationException("Incorrect number of salt bytes")

    if len(rawObfuscated) < 2 + saltLength:
        raise SECObfuscationException("Ciphertext corrupt: short salt")

    salt = rawObfuscated[2:saltLength+2]
    cipherText = rawObfuscated[2+saltLength:]

    if len(cipherText) == 0:
        raise SECObfuscationException("Ciphertext corrupt: data short")

    try:
        return impl.deobfuscate(salt, cipherText)
    except ValueError as e:
        raise SECObfuscationException("Ciphertext corrupt: " + str(e))

def obfuscate(embeddedAlgorithmByte, rawPlain, randomGenerator):
    """
    @param embeddedAlgorithmByte - one of ALGO_3DES or ALGO_AES256
    @param rawPlain Original plain text
    @param randomGenerator
    @returns Base64 obfuscated text
    """
    impl = getImplementationFromEmbeddedAlgorithmByte(embeddedAlgorithmByte)
    assert impl is not None

    saltLength = impl.SALT_LENGTH
    salt = randomGenerator.randomBytes(saltLength)
    assert(len(salt) == saltLength)

    rawObfuscated = chr(embeddedAlgorithmByte) + chr(saltLength) + salt + impl.obfuscate(salt, rawPlain)

    import base64
    return base64.b64encode(rawObfuscated)

