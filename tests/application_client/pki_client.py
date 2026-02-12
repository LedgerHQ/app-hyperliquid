"""PKI certificate management for CAL dynamic token testing."""

from enum import IntEnum

from ledgered.devices import DeviceType  # type: ignore[import-untyped]
from ragger.backend.interface import RAPDU, BackendInterface


class CertificatePubKeyUsage(IntEnum):
    """PKI certificate public key usage types."""

    CERTIFICATE_PUBLIC_KEY_USAGE_NONE = 0x00


# PKI certificates for onboarding the CAL mock key on Speculos
# These test certificates have TEST flags, they will be rejected on real devices
PKI_CERTIFICATES = {
    DeviceType.NANOSP: (
    ),
    DeviceType.NANOX: (
    ),
    DeviceType.STAX: (
    ),
    DeviceType.FLEX: (
    ),
    DeviceType.APEX_P: (
    ),
}


class PKIClient:
    """PKI certificate client for CAL dynamic token validation."""

    _CLA: int = 0xB0
    _INS: int = 0x06

    def __init__(self, client: BackendInterface) -> None:
        self._client = client

    def send_certificate(self, key_usage: CertificatePubKeyUsage, payload: bytes) -> RAPDU:
        """Send PKI certificate to device."""
        return self.send_raw(key_usage, payload)

    def send_raw(self, key_usage: CertificatePubKeyUsage, payload: bytes) -> RAPDU:
        """Send raw PKI certificate APDU."""
        header = bytearray()
        header.append(self._CLA)
        header.append(self._INS)
        header.append(key_usage)
        header.append(0x00)
        header.append(len(payload))
        return self._client.exchange_raw(header + payload)
