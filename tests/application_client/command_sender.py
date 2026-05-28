import struct
from collections.abc import Generator
from contextlib import contextmanager
from enum import IntEnum

from ragger.backend.interface import RAPDU, BackendInterface
from ragger.bip import pack_derivation_path

from .pki_client import PKI_CERTIFICATES, CertificatePubKeyUsage, PKIClient
from .tlv import TlvSerializable

MAX_APDU_LEN: int = 255

CLA: int = 0xE0

class InsType(IntEnum):
    GET_ADDRESS = 0x01
    PROVIDE_ACTION_METADATA = 0x02
    SET_ACTION = 0x03
    SIGN_ACTION = 0x04

P1_FIRST:     int = 0x01
P1_FOLLOWING: int = 0x00

def split_message(message: bytes) -> list[bytes]:
    return [message[x:x + MAX_APDU_LEN] for x in range(0, len(message), MAX_APDU_LEN)]


class CommandSender:
    def __init__(self, backend: BackendInterface) -> None:
        self.backend = backend

    def get_address(self, bip32_path: str) -> RAPDU:
        return self.backend.exchange(cla=CLA,
                                     ins=InsType.GET_ADDRESS,
                                     p1=0x00,
                                     p2=0x00,
                                     data=pack_derivation_path(bip32_path))

    def provide_action_metadata(self, obj: TlvSerializable) -> RAPDU:
        cert_apdu = PKI_CERTIFICATES.get(self.backend.device.type)
        if cert_apdu:
            PKIClient(self.backend).send_certificate(
                    CertificatePubKeyUsage.CERTIFICATE_PUBLIC_KEY_USAGE_PERPS_DATA,
                    bytes.fromhex(cert_apdu),
            )
        return self._send_chunked(InsType.PROVIDE_ACTION_METADATA, obj.serialize())

    def set_action(self, obj: TlvSerializable) -> RAPDU:
        return self._send_chunked(InsType.SET_ACTION, obj.serialize())

    def _send_chunked(self, ins: InsType, payload: bytes) -> RAPDU:
        """Send a payload that may span multiple APDUs.

        Prepends the 2-byte big-endian total-length to the payload, then splits
        the result into MAX_APDU_LEN-byte chunks.  The first chunk is sent with
        P1_FIRST (0x01), all subsequent chunks with P1_FOLLOWING (0x00).
        """
        data = struct.pack(">H", len(payload)) + payload
        for i, chunk in enumerate(split_message(data)):
            resp = self.backend.exchange(
                cla=CLA,
                ins=ins,
                p1=P1_FIRST if i == 0 else P1_FOLLOWING,
                p2=0x00,
                data=chunk,
            )
        return resp

    @contextmanager
    def sign_action(self, bip32_path: str) -> Generator[None, None, None]:
        with self.backend.exchange_async(cla=CLA,
                                         ins=InsType.SIGN_ACTION,
                                         p1=0x00,
                                         p2=0x00,
                                         data=pack_derivation_path(bip32_path)) as resp:
            yield resp
