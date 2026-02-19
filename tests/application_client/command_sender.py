from enum import IntEnum

from ragger.backend.interface import RAPDU, BackendInterface
from ragger.bip import pack_derivation_path

MAX_APDU_LEN: int = 255

CLA: int = 0xE0

class InsType(IntEnum):
    GET_ADDRESS = 0x01

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
