import struct

from .tlv import TlvSerializable


class UpdateIsolatedMargin(TlvSerializable):
    asset: int
    is_buy: bool
    ntli: int

    def __init__(self,
                 asset: int,
                 is_buy: bool,
                 ntli: int) -> None:
        self.asset = asset
        self.is_buy = is_buy
        self.ntli = ntli

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0xd1, self.asset)
        payload += self.serialize_field(0xe2, self.is_buy)
        payload += self.serialize_field(0xd6, struct.pack(">q", self.ntli))
        return bytes(payload)
