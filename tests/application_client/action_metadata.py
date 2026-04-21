from enum import IntEnum

from .keychain import Key, sign_data
from .tlv import TlvSerializable


class Network(IntEnum):
    MAINNET = 0
    TESTNET = 1

class OperationType(IntEnum):
    ORDER = 0
    MODIFY = 1
    CANCEL = 2
    UPDATE_LEVERAGE = 3
    CLOSE = 4
    UPDATE_MARGIN = 5
    CANCEL_SL = 6
    CANCEL_TP = 7
    CANCEL_TP_SL = 8

class ActionMetadata(TlvSerializable):
    version: int
    op_type: OperationType
    asset_id: int
    asset_ticker: str
    network: Network
    builder_addr: bytes | None
    margin: int | None
    leverage: int | None
    signature: bytes | None

    def __init__(self,
                 version: int,
                 operation_type: OperationType,
                 asset_id: int,
                 asset_ticker: str,
                 network: Network,
                 builder_addr: bytes | None = None,
                 margin: int | None = None,
                 leverage: int | None = None,
                 signature: bytes | None = None) -> None:
        self.version = version
        self.op_type = operation_type
        self.asset_id = asset_id
        self.asset_ticker = asset_ticker
        self.network = network
        self.builder_addr = builder_addr
        self.margin = margin
        self.leverage = leverage
        self.signature = signature

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0x01, 0x2b)
        payload += self.serialize_field(0x02, self.version)
        payload += self.serialize_field(0xd0, self.op_type)
        payload += self.serialize_field(0xd1, self.asset_id)
        payload += self.serialize_field(0x24, self.asset_ticker)
        payload += self.serialize_field(0xd2, self.network)
        if self.builder_addr is not None:
            payload += self.serialize_field(0xd3, self.builder_addr)
        if self.margin is not None:
            payload += self.serialize_field(0xd4, self.margin)
        if self.leverage is not None:
            payload += self.serialize_field(0xd5, self.leverage)
        sig = self.signature
        if sig is None:
            sig = sign_data(payload, Key.ACTION_METADATA)
        payload += self.serialize_field(0x15, sig)
        return bytes(payload)
