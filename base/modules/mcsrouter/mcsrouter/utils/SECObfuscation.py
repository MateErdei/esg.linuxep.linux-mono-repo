#!/usr/bin/env python
# Copyright 2017 Sophos Plc. All rights reserved.
"""
SECObfuscation Module
"""

from __future__ import absolute_import, print_function, division, unicode_literals

from Crypto.Cipher import DES3
from Crypto.Protocol.KDF import PBKDF2
from Crypto.Hash import SHA512, HMAC


# Algorithm enumeration:
ALGO_3DES = 7
ALGO_AES256 = 8


class SECObfuscationException(Exception):
    """
    SECObfuscationException
    """
    pass


class SECObfuscation(object):
    """
    SECObfuscation
    """

    def get_password(self):
        """
        get_password
        """
        from . import sec_obfuscation_password
        return sec_obfuscation_password.get_password()

    def remove_padding(self, text):
        """
        remove_padding
        """
        padding = ord(text[-1])
        if padding > self.BLOCK_LENGTH:
            raise SECObfuscationException("Padding incorrect")

        return text[:-padding]

    def add_padding(self, text):
        """
        add_padding
        """
        padding = self.BLOCK_LENGTH - len(text) % self.BLOCK_LENGTH
        return text + chr(padding) * padding

    def split_key_iv(self, key_iv):
        """
        split_key_iv
        """
        key = key_iv[0:self.KEY_LENGTH]
        iv_value = key_iv[self.KEY_LENGTH:self.KEY_LENGTH + self.IV_LENGTH]

        return (key, iv_value)

    def deobfuscate(self, salt, cipher_text):
        """
        deobfuscate
        """
        key, iv_value = self.create_session_key(salt)
        cipher = self.create_cipher(key, iv_value)
        return self.remove_padding(cipher.decrypt(cipher_text))

    def obfuscate(self, salt, plain_text):
        """
        obfuscate
        """
        key, iv_value = self.create_session_key(salt)
        cipher = self.create_cipher(key, iv_value)
        return cipher.encrypt(self.add_padding(plain_text))


class ThreeDES(SECObfuscation):
    """
    ThreeDES
    """
    ALGORITHM_MARKER_BYTE = ALGO_3DES
    SALT_LENGTH = 8
    KEY_LENGTH = 24
    BLOCK_LENGTH = 8
    IV_LENGTH = BLOCK_LENGTH

    def create_cipher(self, key, iv_value):
        """
        create_cipher
        """
        return DES3.new(key, DES3.MODE_CBC, IV=iv_value)

    def create_session_key(self, salt):
        """
        @return (key,iv_value) for salt
        """
        import hashlib

        password = self.get_password()

        key_iv = b""
        previous_hash = b""

        while len(key_iv) < self.KEY_LENGTH + self.IV_LENGTH:
            m = hashlib.md5()
            m.update(previous_hash)
            m.update(password)
            m.update(salt)
            previous_hash = m.digest()
            key_iv += previous_hash

        return self.split_key_iv(key_iv)


class AES256(SECObfuscation):
    """
    AES256
    """
    ALGORITHM_MARKER_BYTE = ALGO_AES256
    SALT_LENGTH = 32
    KEY_LENGTH = 256 // 8
    BLOCK_LENGTH = 128 // 8
    IV_LENGTH = BLOCK_LENGTH
    KEY_ITERATIONS = 50000

    def create_cipher(self, key, iv_value):
        """
        create_cipher
        """
        from Crypto.Cipher import AES
        return AES.new(key, AES.MODE_CBC, IV=iv_value)

    def create_session_key(self, salt):
        """
        @return (key,iv_value) for salt
        """

        password = self.get_password()

        def prf(p, s):
            """
            prf
            """
            return HMAC.new(p, s, SHA512).digest()
        key_iv = PBKDF2(password, salt,
                        dkLen=self.KEY_LENGTH + self.IV_LENGTH,
                        count=self.KEY_ITERATIONS,
                        prf=prf
                        )

        return self.split_key_iv(key_iv)


def get_implementation_from_embedded_algorithm_byte(embedded_algorithm_byte):
    """
    get_implementation_from_embedded_algorithm_byte
    """
    if embedded_algorithm_byte == ThreeDES.ALGORITHM_MARKER_BYTE:
        return ThreeDES()
    elif embedded_algorithm_byte == AES256.ALGORITHM_MARKER_BYTE:
        return AES256()

    raise SECObfuscationException("Unknown obfuscation algorithm-id")


def get_implementation(raw_obfuscated):
    """
    get_implementation
    """
    embedded_algorithm_byte = ord(raw_obfuscated[0])
    return get_implementation_from_embedded_algorithm_byte(
        embedded_algorithm_byte)


def deobfuscate(base64_obfuscated):
    """
    @param base64_obfuscated BASE64 encoded obfuscated text
    @returns raw plain text
    """
    import base64
    try:
        raw_obfuscated = base64.b64decode(base64_obfuscated)
    except TypeError:
        raise SECObfuscationException("Invalid Base64 in SECObfuscation")

    impl = get_implementation(raw_obfuscated)
    assert impl is not None

    salt_length = ord(raw_obfuscated[1])
    if salt_length != impl.SALT_LENGTH:
        raise SECObfuscationException("Incorrect number of salt bytes")

    if len(raw_obfuscated) < 2 + salt_length:
        raise SECObfuscationException("Ciphertext corrupt: short salt")

    salt = raw_obfuscated[2:salt_length + 2]
    cipher_text = raw_obfuscated[2 + salt_length:]

    if not cipher_text:
        raise SECObfuscationException("Ciphertext corrupt: data short")

    try:
        return impl.deobfuscate(salt, cipher_text)
    except ValueError as exception:
        raise SECObfuscationException("Ciphertext corrupt: " + str(exception))


def obfuscate(embedded_algorithm_byte, raw_plain, random_generator):
    """
    @param embedded_algorithm_byte - one of ALGO_3DES or ALGO_AES256
    @param raw_plain Original plain text
    @param random_generator
    @returns Base64 obfuscated text
    """
    impl = get_implementation_from_embedded_algorithm_byte(
        embedded_algorithm_byte)
    assert impl is not None

    salt_length = impl.SALT_LENGTH
    salt = random_generator.random_bytes(salt_length)
    assert(len(salt) == salt_length)

    raw_obfuscated = chr(embedded_algorithm_byte) + \
        chr(salt_length) + salt + impl.obfuscate(salt, raw_plain)

    import base64
    return base64.b64encode(raw_obfuscated)
