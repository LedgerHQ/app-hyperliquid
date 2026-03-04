from enum import IntEnum

from .order_request import OrderRequest
from .tlv import TlvSerializable


class Grouping(IntEnum):
    NA = 0x00
    NORMAL_TPSL = 0x01
    POSITION_TPSL = 0x02

class BuilderInfo(TlvSerializable):
    builder_addr: bytes
    builder_fee: int

    def __init__(self, builder_addr: bytes, builder_fee: int) -> None:
        self.builder_addr = builder_addr
        self.builder_fee = builder_fee

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0xd3, self.builder_addr)
        payload += self.serialize_field(0xec, self.builder_fee)
        return bytes(payload)

class BulkOrder(TlvSerializable):
    orders: list[OrderRequest]
    grouping: Grouping
    builder: BuilderInfo | None

    def __init__(self,
                 orders: list[OrderRequest],
                 grouping: Grouping,
                 builder: BuilderInfo | None = None) -> None:
        self.orders = orders
        self.grouping = grouping
        self.builder = builder

    def serialize(self) -> bytes:
        payload = bytearray()
        for order in self.orders:
            payload += self.serialize_field(0xdd, order.serialize())
        payload += self.serialize_field(0xea, self.grouping)
        if self.builder is not None:
            payload += self.serialize_field(0xeb, self.builder.serialize())
        return bytes(payload)
