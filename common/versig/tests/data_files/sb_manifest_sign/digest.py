"""
Openssl digest signing.
"""
import base64
import logging

from . import processutil
from . import DigestCert, DigestConfig

_LOGGER = logging.getLogger(__name__)


class Digest:
    def __init__(self, config: DigestConfig):
        self._config = config

    def sign(self, *, signing_key: str, hashing_algorithm: str, data: bytes) -> str:
        """
        Generate a digest signature for the data blob, using the cert provided.
        :param signing_key: The key id to sign with
        :param hashing_algorithm: The hashing algorithm to use
        :param data: The data to be signed
        :return: Signature for the data blob
        """
        cert = self._config.get_certificate(signing_key)
        return self._get_signature(hashing_algorithm=hashing_algorithm, cert=cert, data=data)

    def _get_signature(self, *, hashing_algorithm: str, cert: DigestCert, data: bytes):
        """
        Use openssl to generate a signature for input data.
        """
        options = ['dgst', '-' + hashing_algorithm, '-sign', str(cert.key)]
        if self._config.signing_engine:
            options += ['-engine', self._config.signing_engine, '-keyform', 'engine']
        else:
            options += ['-keyform', 'pem']

        return_code, stdout, stderr = processutil.execute_command(self._config.openssl_path, options, data)

        if return_code != 0:
            raise RuntimeError(
                f"Signing failed.\nOptions:{options}\nReturn Code:{return_code}\nOutput:\n{stdout}\n{stderr}.")

        return base64.b64encode(stdout).decode()

    def get_signing_artifact(self, *, signing_key: str, item_type: str, cert_type: str) -> str:
        """
        Return the certificate for the signing key name passed in.
        :param signing_key: The key for which the cert is required.
        :param cert_type: The cert type to retrieve
        :param item_type: Identify the type of item associated with the cert type to retrieve
        :return str: Certificate content
        """
        cert = self._config.get_certificate(signing_key)
        if item_type == 'cert':
            path = self._get_cert_path(cert, cert_type)
        elif item_type == 'crl':
            path = self._get_crl_path(cert, cert_type)
        else:
            raise ValueError(f'Unknown item type: {item_type}')

        if not path:
            raise ValueError(f'No item available for key={signing_key}, item_type={item_type}, cert_type={cert_type}')
        return open(path, 'r').read().replace('\r\n', '\n')

    @staticmethod
    def _get_crl_path(cert: DigestCert, cert_type: str):
        if not cert.crls:
            raise ValueError('No CRLs available')
        if cert_type == 'ca':
            path = cert.crls.inter_crl_path
        elif cert_type == 'root':
            path = cert.crls.root_crl_path
        else:
            raise ValueError(f'Unknown item type: {cert_type}')
        return path

    @staticmethod
    def _get_cert_path(cert: DigestCert, cert_type: str):
        if cert_type == 'key':
            raise ValueError('Cannot retrieve the signing key')
        if cert_type == 'pub':
            if cert.cert_path:
                path = cert.cert_path
            else:
                raise ValueError('Missing cert_path value for cert')
        elif cert_type == 'ca':
            if cert.inter_ca_path:
                path = cert.inter_ca_path
            else:
                raise ValueError('Missing inter_ca_path value for cert')
        elif cert_type == 'root':
            if cert.root_ca_path:
                path = cert.root_ca_path
            else:
                raise ValueError('Missing root_ca_path value for cert')
        else:
            raise ValueError(f'Unknown item type: {cert_type}')
        return path
