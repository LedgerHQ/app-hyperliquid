from enum import IntEnum

from .tlv import TlvSerializable


class Abstraction(IntEnum):
    DISABLED = 0x00
    UNIFIED_ACCOUNT = 0x01
    PORTFOLIO_MARGIN = 0x02


class UserSetAbstraction(TlvSerializable):
    signature_chain_id: int
    abstraction: Abstraction

    def __init__(self, signature_chain_id: int, abstraction: Abstraction) -> None:
        self.signature_chain_id = signature_chain_id
        self.abstraction = abstraction

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0x23, self.signature_chain_id)
        payload += self.serialize_field(0xDF, self.abstraction)
        return bytes(payload)
