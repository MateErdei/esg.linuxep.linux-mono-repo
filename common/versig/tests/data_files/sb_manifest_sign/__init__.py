from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional, Union

@dataclass
class DigestCrls:
    root_crl_path: Path
    inter_crl_path: Optional[Path]


DigestKey = Union[Path]


@dataclass
class DigestCert:
    key: DigestKey
    cert_path: Path
    root_ca_path: Optional[Path] = None
    inter_ca_path: Optional[Path] = None
    crls: Optional[DigestCrls] = None


class DigestConfig(ABC):
    @property
    @abstractmethod
    def certificates(self) -> Dict[str, DigestCert]:
        pass

    @property
    @abstractmethod
    def signing_engine(self) -> Optional[str]:
        pass

    @property
    @abstractmethod
    def openssl_path(self) -> str:
        pass

    def get_certificate(self, signing_key: str) -> DigestCert:
        try:
            return self.certificates[signing_key.upper()]
        except KeyError:
            raise ValueError(f'Unknown key: "{signing_key}", available keys: {self.certificates.keys()}')
