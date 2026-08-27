#!/usr/bin/env python3
"""Hyperliquid seed corpus.

One well-formed APDU per INS, to carry the fuzzer past the discriminator
gates and into the TLV parsers. Each seed is Absolution's zero prefix, the
four control bytes, then the payload: the seed supplies payload only and
never writes state, so there is no prefix layout to track.
"""

import os
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
SDK_SCRIPTS = os.environ.get(
    "LEDGER_FUZZ_SCRIPTS",
    os.path.join(os.environ.get("BOLOS_SDK", ""), "fuzzing", "scripts"),
)
sys.path.insert(0, SDK_SCRIPTS)

from fuzz_seed_utils import (
    resolve_prefix_size,
    resolve_seed_prefix,
    ctrl_bytes,
)


# fuzz_commands[] order in fuzz_dispatcher.c (must stay aligned).
CMD_GET_ADDRESS = 0
CMD_PROVIDE_METADATA = 1
CMD_SET_ACTION = 2
CMD_SIGN_ACTION = 3

# Top-level TLV tags
TAG_STRUCT_TYPE = 0x01
TAG_STRUCT_VERSION = 0x02
TAG_SIGNATURE = 0x15
TAG_SIGNATURE_CHAIN_ID = 0x23
TAG_ASSET_TICKER = 0x24
TAG_MAX_FEE_RATE = 0xB0
TAG_OPERATION_TYPE = 0xD0
TAG_ACTION_TYPE = 0xD0  # same byte, different scope
TAG_ASSET_ID = 0xD1
TAG_ASSET = 0xD1
TAG_NETWORK = 0xD2
TAG_BUILDER_ADDR = 0xD3
TAG_MARGIN = 0xD4
TAG_LEVERAGE_META = 0xD5
TAG_NTLI = 0xD6
TAG_ORDER_INNER = 0xD7
TAG_MODIFY_REQUEST = 0xD8
TAG_CANCEL_REQUEST = 0xD9
TAG_NONCE = 0xDA
TAG_ACTION = 0xDB
TAG_OID = 0xDC
TAG_ORDER = 0xDD
TAG_IS_CROSS = 0xDE
TAG_ORDER_TYPE = 0xE0
TAG_IS_BUY = 0xE2
TAG_LIMIT_PX = 0xE3
TAG_SZ = 0xE4
TAG_REDUCE_ONLY = 0xE5
TAG_TIF = 0xE6
TAG_IS_MARKET = 0xE7
TAG_TRIGGER_PX = 0xE8
TAG_TRIGGER_TYPE = 0xE9
TAG_GROUPING = 0xEA
TAG_BUILDER = 0xEB
TAG_BUILDER_FEE = 0xEC
TAG_LEVERAGE_INNER = 0xED

# Struct types for the metadata vs action wrappers (compile-time magic bytes).
STRUCT_TYPE_METADATA = 0x2B
STRUCT_TYPE_ACTION = 0x2C
STRUCT_VERSION_1 = 0x01

# e_operation_type
OP_TYPE_ORDER = 0
OP_TYPE_MODIFY = 1
OP_TYPE_CANCEL = 2
OP_TYPE_CLOSE = 4
OP_TYPE_UPDATE_MARGIN = 5
OP_TYPE_CANCEL_SL = 6
OP_TYPE_CANCEL_TP = 7
OP_TYPE_CANCEL_TP_SL = 8

# e_action_type
ACTION_TYPE_BULK_ORDER = 0
ACTION_TYPE_BULK_MODIFY = 1
ACTION_TYPE_BULK_CANCEL = 2
ACTION_TYPE_UPDATE_LEVERAGE = 3
ACTION_TYPE_APPROVE_BUILDER_FEE = 4
ACTION_TYPE_UPDATE_ISOLATED_MARGIN = 5

# e_order_type / e_tif / e_trigger_type / e_grouping
ORDER_TYPE_LIMIT = 0
ORDER_TYPE_TRIGGER = 1
TIF_GTC = 2
TRIGGER_TYPE_TP = 0
GROUPING_NA = 0

# Default values shared across seeds. Any single-byte / fixed-length tag uses
# these; nothing here needs to satisfy semantic invariants beyond parsing.
DEFAULT_ASSET_ID = 1
DEFAULT_NONCE = 0x0102030405060708
DEFAULT_BUILDER = bytes(range(20))
SIGNATURE_70B = bytes(70)


# ── Prefix builder ──────────────────────────────────────────────────────────


def build_head(base_prefix, *, p1=0, p2=0,
               has_metadata=False, op_type=0, asset_id=DEFAULT_ASSET_ID,
               cmd_idx=0, action_count=0, action_index=0, network=0,
               crypto_fail=0, nbgl_reject=0,
               action_types=None, action_nonces=None,
               signing_path=None, signing_path_length=None):
    """Return the prefix plus control bytes that precede the APDU payload.

    State keyword arguments are accepted and ignored: they record which scenario
    a call site intended, but the invariant owns that state. Only cmd_idx, p1 and
    p2 are honoured, being fuzzer control bytes rather than application state.
    """
    del (has_metadata, op_type, asset_id, action_count, action_index, network,
         crypto_fail, nbgl_reject, action_types, action_nonces,
         signing_path, signing_path_length)
    return bytes(base_prefix) + ctrl_bytes(True, cmd_idx, p1, p2)


# ── TLV helpers ─────────────────────────────────────────────────────────────


def der_encode(value):
    """Encode an unsigned integer as the SDK's DER variant.

    The Ledger TLV library (lib_tlv/tlv_library.c) uses DER long-form for
    values >= 0x80: first byte has the high bit set and encodes the number
    of following big-endian bytes. Values < 0x80 are stored verbatim.
    """
    n = max(1, (value.bit_length() + 7) // 8)
    body = value.to_bytes(n, "big")
    if value >= 0x80:
        return bytes([0x80 | n]) + body
    return body


def tlv(tag, value):
    if isinstance(value, int):
        # Auto-size small ints as little-endian. Single-byte enum values are
        # the common case; sizes >= 2 are explicit elsewhere.
        value = bytes([value & 0xFF])
    return der_encode(tag) + der_encode(len(value)) + value


def tlv_u32(tag, v):
    # lib_tlv/tlv_library.c reads scalars big-endian (U8BE) with left-padding.
    return tlv(tag, struct.pack(">I", v & 0xFFFFFFFF))


def tlv_u64(tag, v):
    return tlv(tag, struct.pack(">Q", v & 0xFFFFFFFFFFFFFFFF))


def tlv_bool(tag, v):
    return tlv(tag, bytes([1 if v else 0]))


def tlv_str(tag, s):
    return tlv(tag, s.encode("ascii"))


def tlv_bytes(tag, b):
    return tlv(tag, b)


# ── Inner TLV builders (action body) ────────────────────────────────────────


def build_limit_order():
    return tlv(TAG_TIF, TIF_GTC)


def build_trigger_order():
    return (
        tlv_bool(TAG_IS_MARKET, False)
        + tlv_str(TAG_TRIGGER_PX, "100.0")
        + tlv(TAG_TRIGGER_TYPE, TRIGGER_TYPE_TP)
    )


def build_order_request(*, order_type=ORDER_TYPE_LIMIT, asset=DEFAULT_ASSET_ID):
    inner = build_limit_order() if order_type == ORDER_TYPE_LIMIT else build_trigger_order()
    return (
        tlv(TAG_ORDER_TYPE, order_type)
        + tlv_u32(TAG_ASSET, asset)
        + tlv_bool(TAG_IS_BUY, True)
        + tlv_str(TAG_LIMIT_PX, "100.0")
        + tlv_str(TAG_SZ, "1.0")
        + tlv_bool(TAG_REDUCE_ONLY, False)
        + tlv_bytes(TAG_ORDER_INNER, inner)
    )


def build_builder_info():
    return (
        tlv_bytes(TAG_BUILDER_ADDR, DEFAULT_BUILDER)
        + tlv_u64(TAG_BUILDER_FEE, 100)
    )


def build_bulk_order(*, with_builder=False, order_type=ORDER_TYPE_LIMIT):
    body = (
        tlv_bytes(TAG_ORDER, build_order_request(order_type=order_type))
        + tlv(TAG_GROUPING, GROUPING_NA)
    )
    if with_builder:
        body += tlv_bytes(TAG_BUILDER, build_builder_info())
    return body


def build_modify_request():
    return (
        tlv_bytes(TAG_ORDER, build_order_request())
        + tlv_u64(TAG_OID, 12345)
    )


def build_bulk_modify():
    return tlv_bytes(TAG_MODIFY_REQUEST, build_modify_request())


def build_cancel_request(*, asset=DEFAULT_ASSET_ID):
    return tlv_u32(TAG_ASSET, asset) + tlv_u64(TAG_OID, 12345)


def build_bulk_cancel():
    return tlv_bytes(TAG_CANCEL_REQUEST, build_cancel_request())


def build_update_leverage():
    return (
        tlv_u32(TAG_ASSET, DEFAULT_ASSET_ID)
        + tlv_bool(TAG_IS_CROSS, False)
        + tlv_u32(TAG_LEVERAGE_INNER, 3)
    )


def build_approve_builder_fee():
    return (
        tlv_u64(TAG_SIGNATURE_CHAIN_ID, 0x66)  # arbitrary
        + tlv_str(TAG_MAX_FEE_RATE, "0.50%")
        + tlv_bytes(TAG_BUILDER_ADDR, DEFAULT_BUILDER)
    )


def build_update_isolated_margin(*, asset=DEFAULT_ASSET_ID):
    return (
        tlv_u32(TAG_ASSET, asset)
        + tlv_bool(TAG_IS_BUY, True)
        + tlv_u64(TAG_NTLI, 100)
    )


# ── Top-level wrappers ──────────────────────────────────────────────────────


def build_action_envelope(action_type, body):
    """Wrap an action body in the outer ACTION TLV (struct_type 0x2c)."""
    return (
        tlv(TAG_STRUCT_TYPE, STRUCT_TYPE_ACTION)
        + tlv(TAG_STRUCT_VERSION, STRUCT_VERSION_1)
        + tlv(TAG_ACTION_TYPE, action_type)
        + tlv_u64(TAG_NONCE, DEFAULT_NONCE)
        + tlv_bytes(TAG_ACTION, body)
    )


def build_metadata_envelope(op_type, *, with_optional=False, asset_id=DEFAULT_ASSET_ID):
    parts = (
        tlv(TAG_STRUCT_TYPE, STRUCT_TYPE_METADATA)
        + tlv(TAG_STRUCT_VERSION, STRUCT_VERSION_1)
        + tlv(TAG_OPERATION_TYPE, op_type)
        + tlv_u32(TAG_ASSET_ID, asset_id)
        + tlv_str(TAG_ASSET_TICKER, "BTC")
        + tlv(TAG_NETWORK, 0)  # NETWORK_MAINNET
    )
    if with_optional:
        parts += tlv_bytes(TAG_BUILDER_ADDR, DEFAULT_BUILDER)
        parts += tlv_u64(TAG_MARGIN, 500)
        parts += tlv_u32(TAG_LEVERAGE_META, 5)
    parts += tlv_bytes(TAG_SIGNATURE, SIGNATURE_70B)
    return parts


SIGN_PATH = [0x8000002C, 0x8000003C, 0x80000000, 0x00000000, 0x00000000]


def build_bip32_path(path=None):
    """A BIP32 path APDU body: [u8 segment_count][BE u32 segments...]."""
    if path is None:
        path = SIGN_PATH
    blob = bytes([len(path)])
    for level in path:
        blob += struct.pack(">I", level)
    return blob


def build_signing_path_blob_path(path=None):
    """Pack a BIP32 path into a 10-segment, big-endian blob for g_signing_ctx.

    The handler memcmp's the full s_signing_ctx struct (10 segments × 4 bytes
    plus length plus padding) against the APDU-derived `tmp`. Segments beyond
    `len(path)` stay zero in both, so as long as the prefix mirrors what
    handler_sign_action() wrote into `tmp` the comparison passes.
    """
    if path is None:
        path = SIGN_PATH
    return list(path)


# ── APDU framing ────────────────────────────────────────────────────────────


def build_apdu_payload_tlv(tlv_body):
    """SET_ACTION / PROVIDE_METADATA: [u16 BE length][TLV]."""
    return struct.pack(">H", len(tlv_body)) + tlv_body


# ── Seed factories ──────────────────────────────────────────────────────────


def make_seed_metadata(base_prefix, name, op_type, *, with_optional=False):
    # PROVIDE_ACTION_METADATA is the entry point that creates g_ctx.metadata
    # from the APDU body, so start with has_metadata=False (the natural
    # pre-call state). op_type/asset_id in the prefix are ignored on this
    # path; the parser writes them from TAG_OPERATION_TYPE / TAG_ASSET_ID.
    head = build_head(base_prefix, p1=1, has_metadata=False, op_type=0,
                      cmd_idx=CMD_PROVIDE_METADATA)
    body = build_metadata_envelope(op_type, with_optional=with_optional)
    return name, head + build_apdu_payload_tlv(body)


def make_seed_action(base_prefix, name, op_type, action_type, body):
    head = build_head(base_prefix, p1=1, has_metadata=True, op_type=op_type,
                      cmd_idx=CMD_SET_ACTION)
    envelope = build_action_envelope(action_type, body)
    return name, head + build_apdu_payload_tlv(envelope)


def make_seed_get_address(base_prefix):
    head = build_head(base_prefix, cmd_idx=CMD_GET_ADDRESS)
    return "get_address_p2pkh", head + build_bip32_path()


# Compatibility table: which op_type each action type is paired with under
# is_action_compatible() in src/action.c. Mirrors the switch in that file so
# the seeds we generate route to the right ui_*/eip712 path.
_OP_FOR_ACTION = {
    ACTION_TYPE_BULK_ORDER:           OP_TYPE_ORDER,
    ACTION_TYPE_BULK_MODIFY:          OP_TYPE_MODIFY,
    ACTION_TYPE_BULK_CANCEL:          OP_TYPE_CANCEL,
    ACTION_TYPE_UPDATE_LEVERAGE:      OP_TYPE_ORDER,
    ACTION_TYPE_APPROVE_BUILDER_FEE:  OP_TYPE_ORDER,
    ACTION_TYPE_UPDATE_ISOLATED_MARGIN: OP_TYPE_UPDATE_MARGIN,
}


def make_seed_sign_action_eip712(base_prefix, action_type, *, network=0, path=None):
    """Drive SIGN_ACTION past handler_sign_action()'s memcmp gate.

    The non-first SIGN_ACTION branch is the only path that reaches
    action_hash() → {eip712_builder_fee_hash, compute_connection_id +
    eip712_cid_hash} → eip712_sign(). It runs when action_index > 0 (so
    ctx_current_action_is_first() returns false) AND memcmp(tmp, g_signing_ctx)
    succeeds. We patch the prefix to put a fully-formed g_signing_ctx in place
    and send a SIGN_ACTION APDU with the same path bytes; the prefix also
    seats an action of the requested type at actions[action_index] so that
    ctx_get_current_action() returns non-NULL.

    The action's union body remains zero (Absolution does not fuzz it), which
    is fine for action_hash: BULK_* / UPDATE_* serialize an empty payload and
    eip712_builder_fee_hash takes its inputs from zeroed fields. The point is
    to drive code, not produce a valid signature.

    fuzz_mock_crypto_fail / fuzz_mock_nbgl_reject are forced to 0 so the
    keccak path inside eip712_*_hash actually runs (otherwise it short-
    circuits on the first cx_keccak_init_no_throw failure).
    """
    if path is None:
        path = SIGN_PATH
    op_type = _OP_FOR_ACTION[action_type]
    # Place the action at index 1; populate slot 0 with a different/compatible
    # type so ctx_push_action's "no duplicate types" rule is consistent.
    if action_type == ACTION_TYPE_BULK_ORDER:
        placeholder = ACTION_TYPE_APPROVE_BUILDER_FEE
    else:
        placeholder = ACTION_TYPE_BULK_ORDER
    action_types = [placeholder, action_type]
    head = build_head(
        base_prefix,
        has_metadata=True,
        op_type=op_type,
        cmd_idx=CMD_SIGN_ACTION,
        action_count=2,
        action_index=1,
        action_types=action_types,
        action_nonces=[DEFAULT_NONCE, DEFAULT_NONCE],
        signing_path=path,
        signing_path_length=len(path),
        crypto_fail=0,
        nbgl_reject=0,
    )
    return head + build_bip32_path(path)


def _flip_crypto_fail(blob):
    """Previously patched a prefix byte to force fuzz_mock_crypto_fail=1.

    That switch is an Absolution-sampled global, so the fuzzer reaches it by
    sampling rather than by a seed poking a hardcoded offset. Constrain the
    symbol's domain in invariants/ if those error branches need more weight.
    """
    return blob


def make_seed_sign_action(base_prefix):
    # Legacy seed kept for the first-action path coverage in handler_sign_action.
    # It exits at SWO_INCORRECT_DATA after ctx_get_action_metadata fails because
    # the action list is empty, but still touches the BIP32 read + ctx checks.
    head = build_head(base_prefix, has_metadata=True, op_type=OP_TYPE_ORDER,
                      cmd_idx=CMD_SIGN_ACTION)
    return "sign_action_bip32", head + build_bip32_path()


def generate_seeds(output_dir):
    os.makedirs(output_dir, exist_ok=True)

    prefix_size = resolve_prefix_size()
    base_prefix = resolve_seed_prefix(prefix_size)

    seeds = []

    # Metadata envelopes — one per op_type. Helps the fuzzer learn what a
    # well-formed PROVIDE_ACTION_METADATA APDU looks like; downstream mutations
    # then explore field corruption / missing tags / oversize buffers.
    metadata_ops = [
        ("metadata_order",            OP_TYPE_ORDER),
        ("metadata_modify",           OP_TYPE_MODIFY),
        ("metadata_cancel",           OP_TYPE_CANCEL),
        ("metadata_close",            OP_TYPE_CLOSE),
        ("metadata_update_margin",    OP_TYPE_UPDATE_MARGIN),
        ("metadata_cancel_sl",        OP_TYPE_CANCEL_SL),
        ("metadata_cancel_tp",        OP_TYPE_CANCEL_TP),
        ("metadata_cancel_tp_sl",     OP_TYPE_CANCEL_TP_SL),
    ]
    for name, op_type in metadata_ops:
        seeds.append(make_seed_metadata(base_prefix, name, op_type))
    seeds.append(make_seed_metadata(base_prefix, "metadata_order_full",
                                    OP_TYPE_ORDER, with_optional=True))

    # Action envelopes — one per sub-action type. The prefix is patched so the
    # action_compatibility check inside parse_action() passes; otherwise the
    # sub-parser would never be invoked.
    seeds.append(make_seed_action(
        base_prefix, "action_bulk_order_limit",
        OP_TYPE_ORDER, ACTION_TYPE_BULK_ORDER,
        build_bulk_order(order_type=ORDER_TYPE_LIMIT)))
    seeds.append(make_seed_action(
        base_prefix, "action_bulk_order_trigger",
        OP_TYPE_ORDER, ACTION_TYPE_BULK_ORDER,
        build_bulk_order(order_type=ORDER_TYPE_TRIGGER)))
    seeds.append(make_seed_action(
        base_prefix, "action_bulk_order_with_builder",
        OP_TYPE_ORDER, ACTION_TYPE_BULK_ORDER,
        build_bulk_order(with_builder=True)))
    seeds.append(make_seed_action(
        base_prefix, "action_bulk_modify",
        OP_TYPE_MODIFY, ACTION_TYPE_BULK_MODIFY,
        build_bulk_modify()))
    seeds.append(make_seed_action(
        base_prefix, "action_bulk_order_via_modify",
        OP_TYPE_MODIFY, ACTION_TYPE_BULK_ORDER,
        build_bulk_order()))
    for op in (OP_TYPE_CANCEL, OP_TYPE_CANCEL_SL,
               OP_TYPE_CANCEL_TP, OP_TYPE_CANCEL_TP_SL):
        seeds.append(make_seed_action(
            base_prefix, f"action_bulk_cancel_op{op}",
            op, ACTION_TYPE_BULK_CANCEL, build_bulk_cancel()))
    seeds.append(make_seed_action(
        base_prefix, "action_update_leverage",
        OP_TYPE_ORDER, ACTION_TYPE_UPDATE_LEVERAGE,
        build_update_leverage()))
    seeds.append(make_seed_action(
        base_prefix, "action_approve_builder_fee",
        OP_TYPE_ORDER, ACTION_TYPE_APPROVE_BUILDER_FEE,
        build_approve_builder_fee()))
    seeds.append(make_seed_action(
        base_prefix, "action_update_isolated_margin",
        OP_TYPE_UPDATE_MARGIN, ACTION_TYPE_UPDATE_ISOLATED_MARGIN,
        build_update_isolated_margin()))
    seeds.append(make_seed_action(
        base_prefix, "action_close_bulk_order",
        OP_TYPE_CLOSE, ACTION_TYPE_BULK_ORDER, build_bulk_order()))

    # BIP32-path commands (GET_ADDRESS, SIGN_ACTION).
    seeds.append(make_seed_get_address(base_prefix))
    seeds.append(make_seed_sign_action(base_prefix))

    # SIGN_ACTION non-first-action paths. These pre-populate g_signing_ctx
    # so the memcmp gate is satisfied and action_hash + eip712_*_hash +
    # eip712_sign run on each action variant. Two paths are generated
    # per action type (mainnet/testnet) because the network-string branch
    # inside compute_connection_id / eip712_builder_fee_hash differs.
    eip712_action_types = [
        ("bulk_order",            ACTION_TYPE_BULK_ORDER),
        ("bulk_modify",           ACTION_TYPE_BULK_MODIFY),
        ("bulk_cancel",           ACTION_TYPE_BULK_CANCEL),
        ("update_leverage",       ACTION_TYPE_UPDATE_LEVERAGE),
        ("approve_builder_fee",   ACTION_TYPE_APPROVE_BUILDER_FEE),
        ("update_isolated_margin",ACTION_TYPE_UPDATE_ISOLATED_MARGIN),
    ]
    for name, atype in eip712_action_types:
        seeds.append((
            f"sign_action_eip712_{name}",
            make_seed_sign_action_eip712(base_prefix, atype)))
        seeds.append((
            f"sign_action_eip712_{name}_short",
            make_seed_sign_action_eip712(base_prefix, atype,
                                         path=[0x8000002C])))

    # crypto-failure variants: exercise the `if (cx_*_init_no_throw(...)
    # != CX_OK) return false;` error paths inside eip712_*.c. Without a
    # seed that explicitly sets fuzz_mock_crypto_fail=1 these lines are
    # only discovered by random mutation of the selector byte. One per
    # action type is enough since the gate is the same for all of them.
    seeds.append((
        "sign_action_eip712_builder_fee_cryptofail",
        _flip_crypto_fail(make_seed_sign_action_eip712(
            base_prefix, ACTION_TYPE_APPROVE_BUILDER_FEE))))
    seeds.append((
        "sign_action_eip712_bulk_order_cryptofail",
        _flip_crypto_fail(make_seed_sign_action_eip712(
            base_prefix, ACTION_TYPE_BULK_ORDER))))

    written = 0
    for name, blob in seeds:
        path = os.path.join(output_dir, f"custom_{name}")
        with open(path, "wb") as f:
            f.write(blob)
        written += 1

    print(f"Generated {written} Hyperliquid custom seed files in {output_dir} "
          f"(prefix size {prefix_size})")


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "base-corpus"
    generate_seeds(out)
