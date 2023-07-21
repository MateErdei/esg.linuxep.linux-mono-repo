import os
from pathlib import Path
import platform
from typing import Dict, Optional

from . import DigestCert, DigestConfig, DigestCrls

DEV_CERT_DIR = Path(__file__).parent / 'devcerts'

MANIFEST_SHA1_CERT_ID = DEV_CERT_DIR / 'manifest_sha1_private_key.pem'
MANIFEST_SHA1_PUB_CERT_PATH = DEV_CERT_DIR / 'manifest_sha1_public_cert.crt'
MANIFEST_SHA1_CA_CERT_PATH = DEV_CERT_DIR / 'manifest_sha1_ca_cert.crt'
MANIFEST_SHA1_ROOT_CERT_PATH = DEV_CERT_DIR / 'manifest_sha1_root_cert.crt'
MANIFEST_SHA1_CA_CRL = DEV_CERT_DIR / 'manifest_sha1_ca.crl'
MANIFEST_SHA1_ROOT_CRL = DEV_CERT_DIR / 'manifest_sha1_root.crl'

MANIFEST_SHA384_CERT_ID = DEV_CERT_DIR / 'manifest_sha384_private_key.pem'
MANIFEST_SHA384_PUB_CERT_PATH = DEV_CERT_DIR / 'manifest_sha384_public_cert.crt'
MANIFEST_SHA384_CA_CERT_PATH = DEV_CERT_DIR / 'manifest_sha384_ca_cert.crt'
MANIFEST_SHA384_ROOT_CERT_PATH = DEV_CERT_DIR / 'manifest_sha384_root_cert.crt'
MANIFEST_SHA384_CA_CRL = DEV_CERT_DIR / 'manifest_sha384_ca.crl'
MANIFEST_SHA384_ROOT_CRL = DEV_CERT_DIR / 'manifest_sha384_root.crl'

MANIFEST_CERTS = {
    'MANIFEST_SHA1': DigestCert(
        key=MANIFEST_SHA1_CERT_ID,
        cert_path=MANIFEST_SHA1_PUB_CERT_PATH,
        root_ca_path=MANIFEST_SHA1_ROOT_CERT_PATH,
        inter_ca_path=MANIFEST_SHA1_CA_CERT_PATH,
        crls=DigestCrls(
            root_crl_path=MANIFEST_SHA1_ROOT_CRL,
            inter_crl_path=MANIFEST_SHA1_CA_CRL
        )
    ),
    'MANIFEST_SHA384': DigestCert(
        key=MANIFEST_SHA384_CERT_ID,
        cert_path=MANIFEST_SHA384_PUB_CERT_PATH,
        root_ca_path=MANIFEST_SHA384_ROOT_CERT_PATH,
        inter_ca_path=MANIFEST_SHA384_CA_CERT_PATH,
        crls=DigestCrls(
            root_crl_path=MANIFEST_SHA384_ROOT_CRL,
            inter_crl_path=MANIFEST_SHA384_CA_CRL
        )
    ),
    "LOCAL_EMPTY": DigestCert(
        key="cert_files/empty_valid/signing.key.pem",
        cert_path="cert_files/empty_valid/signing.crt",
        root_ca_path="cert_files/empty_valid/rootca.crt",
        inter_ca_path="cert_files/empty_valid/inter.crt"
    )
}


class LocalDigestConfig(DigestConfig):
    @property
    def certificates(self) -> Dict[str, DigestCert]:
        return MANIFEST_CERTS

    @property
    def signing_engine(self) -> Optional[str]:
        return None

    @property
    def openssl_path(self) -> str:
        return _find_local_openssl_installation()


# Get the expected path to openssl on the system
def _find_local_openssl_installation():
    # if we are a windows system then we look in the folder that openssl.light or openssl from chocolatey will be.
    if platform.system() == 'Windows':
        expected_paths = ['C:/Program Files/OpenSSL/bin/openssl.exe',
                          'C:/Program Files/OpenSSL-Win64/bin/openssl.exe']
    else:
        # if this is not windows then we look in the standard path for linux.
        expected_paths = ['/usr/bin/openssl']

    # Fall back to the environment variable, if all else fails.
    if 'OPENSSL_PATH' in os.environ:
        expected_paths.append(os.environ['OPENSSL_PATH'])

    for openssl_path in expected_paths:
        if os.path.isfile(openssl_path):
            return openssl_path

    # Ok, we have a problem.
    if platform.system() == "Windows":
        raise RuntimeError("PLEASE INSTALL OpenSSL.Light from chocolatey.")
    raise RuntimeError("Local OpenSSL installation was not found in [/usr/bin/openssl]")
