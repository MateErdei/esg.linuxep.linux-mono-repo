#!/usr/bin/env python
# Copyright 2017 Sophos Plc. All rights reserved.
"""
sec_obfuscation Module
"""
#pylint: disable=no-self-use


import binascii


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


class SECObfuscation:
    """
    SECObfuscation class
    """
    KEY_LENGTH = None
    BLOCK_LENGTH = None
    IV_LENGTH = BLOCK_LENGTH

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
        padding = int(text[-1])
        if padding > self.BLOCK_LENGTH:
            raise SECObfuscationException("Padding incorrect")

        return text[:-padding]

    def add_padding(self, text):
        """
        add_padding
        """
        padding = self.BLOCK_LENGTH - len(text) % self.BLOCK_LENGTH
        return text + bytes([padding] * padding)

    def split_key_iv(self, key_iv):
        """
        split_key_iv
        """

        key = key_iv[0:self.KEY_LENGTH]
        iv_value = key_iv[self.KEY_LENGTH:self.KEY_LENGTH + self.IV_LENGTH]

        return (key, iv_value)

    def create_session_key(self, salt):
        """
        create_session_key
        """
        raise NotImplementedError()

    def create_cipher(self, key, iv_value):
        """
        create_cipher
        """
        raise NotImplementedError()

    def deobfuscate(self, salt, cipher_text):
        """
        deobfuscate
        """
        key, iv_value = self.create_session_key(salt)

        cipher = self.create_cipher(key, iv_value)

        as_bytes = self.remove_padding(cipher.decrypt(cipher_text))
        return as_bytes.decode('ascii', 'replace')

    def obfuscate(self, salt, plain_text):
        """
        obfuscate
        """
        if not all([isinstance(salt, (bytearray, bytes)),
                    isinstance(plain_text, (bytearray, bytes))]):
            raise TypeError("Salt and Text must be bytes or bytearray")
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

        key_iv = "".encode("ascii")
        previous_hash = "".encode("ascii")

        while len(key_iv) < self.KEY_LENGTH + self.IV_LENGTH:
            md5_hash = hashlib.md5()
            md5_hash.update(previous_hash)
            md5_hash.update(password)
            md5_hash.update(salt)
            previous_hash = md5_hash.digest()
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

        def pseudo_random_family(password, salt):
            """
            pseudo_random_family
            """
            return HMAC.new(password, salt, SHA512).digest()

        # salt may be type of bytearray, convert to bytes before passing to
        # prevent errors in PBKDF2 call.
        key_iv = PBKDF2(password, bytes(salt),
                        dkLen=self.KEY_LENGTH + self.IV_LENGTH,
                        count=self.KEY_ITERATIONS,
                        prf=pseudo_random_family)

        return self.split_key_iv(key_iv)


def get_embedded_algorithm_byte(embedded_algorithm_byte):
    """
    get_embedded_algorithm_byte
    """
    if embedded_algorithm_byte == ThreeDES.ALGORITHM_MARKER_BYTE:
        return ThreeDES()
    if embedded_algorithm_byte == AES256.ALGORITHM_MARKER_BYTE:
        return AES256()

    raise SECObfuscationException("Unknown obfuscation algorithm-id")


def get_implementation(raw_obfuscated):
    """
    get_implementation
    """
    embedded_algorithm_byte = int(raw_obfuscated[0])
    return get_embedded_algorithm_byte(
        embedded_algorithm_byte)


def deobfuscate(base64_obfuscated):
    """
    @param base64_obfuscated BASE64 encoded obfuscated text
    @returns raw plain text
    """
    import base64
    try:
        raw_obfuscated = base64.b64decode(base64_obfuscated)

    except binascii.Error:
        raise SECObfuscationException("Invalid Base64 in SECObfuscation")

    impl = get_implementation(raw_obfuscated)
    assert impl is not None

    salt_length = int(raw_obfuscated[1])
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
    impl = get_embedded_algorithm_byte(
        embedded_algorithm_byte)
    assert impl is not None

    salt_length = impl.SALT_LENGTH
    salt = random_generator.random_bytes(salt_length)
    assert len(salt) == salt_length

    partial = impl.obfuscate(salt, raw_plain)
    raw_obfuscated_list = bytearray([embedded_algorithm_byte, salt_length]) + salt + \
                          partial
    raw_obfuscated = bytes(raw_obfuscated_list)

    import base64
    return base64.b64encode(raw_obfuscated)
