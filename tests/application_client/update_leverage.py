from .tlv import TlvSerializable


class UpdateLeverage(TlvSerializable):
    asset: int
    is_cross: bool
    leverage: int

    def __init__(self,
                 asset: int,
                 is_cross: bool,
                 leverage: int) -> None:
        self.asset = asset
        self.is_cross = is_cross
        self.leverage = leverage

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0xd1, self.asset)
        payload += self.serialize_field(0xde, self.is_cross)
        payload += self.serialize_field(0xed, self.leverage)
        return bytes(payload)
