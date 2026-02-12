from enum import IntEnum

from ragger.backend.interface import BackendInterface

MAX_APDU_LEN: int = 255

CLA: int = 0xE0

class InsType(IntEnum):
    NONE = 0x00

def split_message(message: bytes) -> list[bytes]:
    return [message[x:x + MAX_APDU_LEN] for x in range(0, len(message), MAX_APDU_LEN)]


class CommandSender:
    def __init__(self, backend: BackendInterface) -> None:
        self.backend = backend
