def unpack_get_address_response(response: bytes) -> bytes:
    assert len(response) == 20
    # the address is sent back as-is, nothing to do
    return response

def unpack_sign_action_response(response: bytes) -> tuple[int, int, bytes, bytes]:
    assert len(response) == 1 + 1 + 32 + 32

    remaining = response[0]
    response = response[1:]

    v = response[0]
    response = response[1:]

    r = response[:32]
    response = response[32:]

    s = response[:32]
    response = response[32:]

    return remaining, v, r, s
