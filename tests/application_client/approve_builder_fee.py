from .tlv import TlvSerializable


class ApproveBuilderFee(TlvSerializable):
    signature_chain_id: int
    max_fee_rate: str
    builder: bytes

    def __init__(self,
                 signature_chain_id: int,
                 max_fee_rate: str,
                 builder: bytes) -> None:
        self.signature_chain_id = signature_chain_id
        self.max_fee_rate = max_fee_rate
        self.builder = builder

    def serialize(self) -> bytes:
        payload = bytearray()
        payload += self.serialize_field(0x23, self.signature_chain_id)
        payload += self.serialize_field(0xb0, self.max_fee_rate)
        payload += self.serialize_field(0xd3, self.builder)
        return bytes(payload)
