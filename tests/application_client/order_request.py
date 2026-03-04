from enum import IntEnum

from .tlv import TlvSerializable


class OrderType(IntEnum):
    LIMIT = 0x00
    TRIGGER = 0x01

class Tif(IntEnum):
    ALO = 0x00
    IOC = 0x01
    GTC = 0x02

class TriggerType(IntEnum):
    TP = 0x00
    SL = 0x01

class OrderRequest(TlvSerializable):
    order_type: OrderType
    asset: int
    is_buy: bool
    limit_px: str
    sz: str
    reduce_only: bool
    tif: Tif | None
    is_market: bool | None
    trigger_px: str | None
    tpsl: TriggerType | None

    def __init__(self,
                 order_type: OrderType,
                 asset: int,
                 is_buy: bool,
                 limit_px: str,
                 sz: str,
                 reduce_only: bool,
                 tif: Tif | None = None,
                 is_market: bool | None = None,
                 trigger_px: str | None = None,
                 tpsl: TriggerType | None = None) -> None:
        self.order_type = order_type
        self.asset = asset
        self.is_buy = is_buy
        self.limit_px = limit_px
        self.sz = sz
        self.reduce_only = reduce_only
        self.tif = tif
        self.is_market = is_market
        self.trigger_px = trigger_px
        self.tpsl = tpsl

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0xe0, self.order_type)
        payload += self.serialize_field(0xe1, self.asset)
        payload += self.serialize_field(0xe2, self.is_buy)
        payload += self.serialize_field(0xe3, self.limit_px)
        payload += self.serialize_field(0xe4, self.sz)
        payload += self.serialize_field(0xe5, self.reduce_only)
        if self.tif is not None:
            payload += self.serialize_field(0xe6, self.tif)
        if self.is_market is not None:
            payload += self.serialize_field(0xe7, self.is_market)
        if self.trigger_px is not None:
            payload += self.serialize_field(0xe8, self.trigger_px)
        if self.tpsl is not None:
            payload += self.serialize_field(0xe9, self.tpsl)
        return bytes(payload)
