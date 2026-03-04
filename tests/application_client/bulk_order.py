from enum import IntEnum

from .order_request import OrderRequest
from .tlv import TlvSerializable


class Grouping(IntEnum):
    NA = 0x00
    NORMAL_TPSL = 0x01
    POSITION_TPSL = 0x02

class BulkOrder(TlvSerializable):
    orders: list[OrderRequest]
    grouping: Grouping
    builder_addr: bytes | None
    builder_fee: int | None

    def __init__(self,
                 orders: list[OrderRequest],
                 grouping: Grouping,
                 builder_addr: bytes | None = None,
                 builder_fee: int | None = None) -> None:
        self.orders = orders
        self.grouping = grouping
        self.builder_addr = builder_addr
        self.builder_fee = builder_fee

    def serialize(self) -> bytes:
        payload = bytearray()
        for order in self.orders:
            payload += self.serialize_field(0xdd, order.serialize())
        payload += self.serialize_field(0xea, self.grouping)
        if self.builder_addr is not None:
            payload += self.serialize_field(0xd3, self.builder_addr)
        if self.builder_fee is not None:
            payload += self.serialize_field(0xec, self.builder_fee)
        return bytes(payload)
