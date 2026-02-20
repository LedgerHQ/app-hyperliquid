from enum import IntEnum


class ActionType(IntEnum):
    ORDER = 0x00
    MODIFY = 0x01
    CANCEL = 0x02
    UPDATE_LEVERAGE = 0x03
