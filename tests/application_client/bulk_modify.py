from .order import Order
from .tlv import TlvSerializable


class BulkModify(TlvSerializable):
    order: Order
    oid: int

    def __init__(self,
                 order: Order,
                 oid: int) -> None:
        self.order = order
        self.oid = oid

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0xdd, self.order.serialize())
        payload += self.serialize_field(0xdc, self.oid)
        return bytes(payload)
