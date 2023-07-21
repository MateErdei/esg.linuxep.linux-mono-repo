"""
Manifest signing.
"""
import logging
import textwrap

from .digest import Digest

_LOGGER = logging.getLogger(__name__)


class Manifest:
    def __init__(self, digest: Digest):
        self._digest = digest

    def sign(self, *, manifest_data: str, include_sha1: bool, include_sha2: bool, signing_key: str) -> str:
        """
        Sign a manifest file with sha384, then (optionally) with sha1.
        :param include_sha1: When set to True then append SHA1 signing information
        :param include_sha2: When set to True then append SHA2 signing information
        :param manifest_data: Manifest data to sign
        :return: Signed manifest data
        """
        assert include_sha1 or include_sha2
        if include_sha2:
            temp_sign_key = signing_key or 'MANIFEST_SHA384'
            sha384_pub_cert = self._digest.get_signing_artifact(signing_key=temp_sign_key, item_type='cert', cert_type='pub')
            sha384_ca_cert = self._digest.get_signing_artifact(signing_key=temp_sign_key, item_type='cert', cert_type='ca')

            # Add the sha384 signature and certs.  These have to have the first and
            # last lines (the ones that mark the beginning and end of a signature or
            # cert block) removed, because the original manifest file format wasn't
            # designed to handle different types of certs, and so the new sha384 data
            # had to be added in a way that didn't confuse old code or cause it to
            # try to process the new certs.  This data gets added to the manifest
            # before the sha1 signatures are generated, so the sha1 signatures are
            # generated over the sha384 data.
            sha384_sig = self._digest.sign(signing_key=temp_sign_key, hashing_algorithm='sha384',
                                           data=manifest_data.encode("UTF-8"))

            manifest_data += '#sig sha384 ' + sha384_sig + '\n'
            manifest_data += '#... cert ' + ''.join(sha384_pub_cert.split('\n')[1:-2]) + '\n'
            manifest_data += '#... cert ' + ''.join(sha384_ca_cert.split('\n')[1:-2]) + '\n'

        # If legacy signing then add the sha1 signature and certs.  These are just appended directly to
        # the manifest.
        if include_sha1:
            temp_sign_key = signing_key or 'MANIFEST_SHA1'
            sha1_pub_cert = self._digest.get_signing_artifact(signing_key=temp_sign_key, item_type='cert', cert_type='pub')
            sha1_ca_cert = self._digest.get_signing_artifact(signing_key=temp_sign_key, item_type='cert', cert_type='ca')
            sha1_sig = self._digest.sign(signing_key=temp_sign_key, hashing_algorithm='sha1', data=manifest_data.encode("UTF-8"))
            manifest_data += "-----BEGIN SIGNATURE-----\n"
            manifest_data += '\n'.join(textwrap.wrap(sha1_sig, width=64))
            manifest_data += "\n-----END SIGNATURE-----\n"
            manifest_data += sha1_pub_cert
            manifest_data += sha1_ca_cert

        return manifest_data
