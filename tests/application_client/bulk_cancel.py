from .tlv import TlvSerializable


class CancelRequest(TlvSerializable):
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

class BulkCancel(TlvSerializable):
    cancels: list[CancelRequest]

    def __init__(self, cancels: list[CancelRequest]) -> None:
        self.cancels = cancels

    def serialize(self) -> bytes:
        payload = bytearray()
        for cancel in self.cancels:
            payload += self.serialize_field(0xd9, cancel.serialize())
        return bytes(payload)
