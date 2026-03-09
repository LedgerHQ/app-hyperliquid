from enum import IntEnum

from .tlv import TlvSerializable


class ActionType(IntEnum):
    BULK_ORDER = 0x00
    BULK_MODIFY = 0x01
    BULK_CANCEL = 0x02
    UPDATE_LEVERAGE = 0x03
    APPROVAL_BUILDER_FEE = 0x04

class SetAction(TlvSerializable):
    version: int
    action_type: ActionType
    nonce: int
    action: TlvSerializable

    def __init__(self,
                 version: int,
                 action_type: ActionType,
                 nonce: int,
                 action: TlvSerializable) -> None:
        self.version = version
        self.action_type = action_type
        self.nonce = nonce
        self.action = action

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0x01, 0x2c)
        payload += self.serialize_field(0x02, self.version)
        payload += self.serialize_field(0xd0, self.action_type)
        payload += self.serialize_field(0xda, self.nonce)
        payload += self.serialize_field(0xdb, self.action.serialize())
        return bytes(payload)
