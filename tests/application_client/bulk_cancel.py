from .tlv import TlvSerializable


class BulkCancel(TlvSerializable):
    asset: int
    oid: int

    def __init__(self,
                 asset: int,
                 oid: int) -> None:
        self.asset = asset
        self.oid = oid

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0xd1, self.asset)
        payload += self.serialize_field(0xdc, self.oid)
        return bytes(payload)
