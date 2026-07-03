from .order_request import OrderRequest
from .tlv import TlvSerializable


class ModifyRequest(TlvSerializable):
    order: OrderRequest
    oid: int

    def __init__(self, order: OrderRequest, oid: int) -> None:
        self.order = order
        self.oid = oid

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0xDD, self.order.serialize())
        payload += self.serialize_field(0xDC, self.oid)
        return bytes(payload)


class BulkModify(TlvSerializable):
    modifies: list[ModifyRequest]

    def __init__(self, modifies: list[ModifyRequest]) -> None:
        self.modifies = modifies

    def serialize(self) -> bytes:
        payload = bytearray()
        for modify in self.modifies:
            payload += self.serialize_field(0xD8, modify.serialize())
        return bytes(payload)
