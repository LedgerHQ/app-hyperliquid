def unpack_get_address_response(response: bytes) -> bytes:
    assert len(response) == 20
    # the address is sent back as-is, nothing to do
    return response
