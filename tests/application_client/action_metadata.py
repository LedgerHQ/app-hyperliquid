from enum import IntEnum

from .action_type import ActionType
from .keychain import Key, sign_data
from .tlv import TlvSerializable


class Network(IntEnum):
    MAINNET = 0
    TESTNET = 1

class ActionMetadata(TlvSerializable):
    version: int
    action_type: ActionType
    asset_id: int
    asset_ticker: str
    network: Network
    builder_addr: bytes | None
    margin: str | None
    signature: bytes | None

    def __init__(self,
                 version: int,
                 action_type: ActionType,
                 asset_id: int,
                 asset_ticker: str,
                 network: Network,
                 builder_addr: bytes | None = None,
                 margin: str | None = None,
                 signature: bytes | None = None) -> None:
        self.version = version
        self.action_type = action_type
        self.asset_id = asset_id
        self.asset_ticker = asset_ticker
        self.network = network
        self.builder_addr = builder_addr
        self.margin = margin
        self.signature = signature

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0x01, 0x2b)
        payload += self.serialize_field(0x02, self.version)
        payload += self.serialize_field(0xd0, self.action_type)
        payload += self.serialize_field(0xd1, self.asset_id)
        payload += self.serialize_field(0x24, self.asset_ticker)
        payload += self.serialize_field(0xd2, self.network)
        if self.builder_addr is not None:
            payload += self.serialize_field(0xd3, self.builder_addr)
        if self.margin is not None:
            payload += self.serialize_field(0xd4, self.margin)
        sig = self.signature
        if sig is None:
            sig = sign_data(payload, Key.ACTION_METADATA)
        payload += self.serialize_field(0x15, sig)
        return bytes(payload)
