"""APDU error-path tests for the Hyperliquid app.

These tests do not involve any UI navigation — they verify that the app correctly
rejects malformed or out-of-sequence APDU commands and returns the appropriate
status words.

Status word constants come from $BOLOS_SDK/include/status_words.h.
"""
import struct

import pytest
from application_client.action_metadata import ActionMetadata, Network, OperationType
from application_client.command_sender import CLA, CommandSender, InsType
from application_client.set_action import ActionType, SetAction
from application_client.update_leverage import UpdateLeverage
from ragger.backend.interface import BackendInterface
from ragger.bip import pack_derivation_path
from ragger.error import ExceptionRAPDU, StatusWords

# ── CLA / INS validation ──────────────────────────────────────────────────────

def test_error_wrong_cla(backend: BackendInterface) -> None:
    """Any command with a wrong CLA must be rejected with SWO_INVALID_CLA (0x6e00)."""
    with pytest.raises(ExceptionRAPDU) as exc_info:
        backend.exchange(cla=0xA0,
                         ins=InsType.GET_ADDRESS,
                         p1=0x00, p2=0x00,
                         data=pack_derivation_path("m/44'/60'/0'/0/0"))
    assert exc_info.value.status == StatusWords.SWO_INVALID_CLA


def test_error_wrong_ins(backend: BackendInterface) -> None:
    """An unknown INS byte must be rejected with SWO_INVALID_INS (0x6d00)."""
    with pytest.raises(ExceptionRAPDU) as exc_info:
        backend.exchange(cla=CLA, ins=0xFF, p1=0x00, p2=0x00, data=b"\x00")
    assert exc_info.value.status == StatusWords.SWO_INVALID_INS


# ── P1 / P2 validation ────────────────────────────────────────────────────────

def test_error_wrong_p1_p2(backend: BackendInterface) -> None:
    """Each command must reject unexpected P1/P2 values with SWO_INCORRECT_P1_P2 (0x6a86).

    Dispatcher checks P1/P2 before data length, so minimal payloads are fine here.
    """
    path_data    = pack_derivation_path("m/44'/60'/0'/0/0")
    two_zero     = struct.pack(">H", 0)  # two-byte length prefix expected by METADATA/SET_ACTION

    # GET_ADDRESS: expects P1=0x00, P2=0x00
    with pytest.raises(ExceptionRAPDU) as exc_info:
        backend.exchange(cla=CLA, ins=InsType.GET_ADDRESS,
                         p1=0x01, p2=0x00, data=path_data)
    assert exc_info.value.status == StatusWords.SWO_INCORRECT_P1_P2

    # PROVIDE_ACTION_METADATA: expects P1=0x01, P2=0x00
    with pytest.raises(ExceptionRAPDU) as exc_info:
        backend.exchange(cla=CLA, ins=InsType.PROVIDE_ACTION_METADATA,
                         p1=0x00, p2=0x00, data=two_zero)
    assert exc_info.value.status == StatusWords.SWO_INCORRECT_P1_P2

    # SET_ACTION: expects P1=0x01, P2=0x00 — test bad P2
    with pytest.raises(ExceptionRAPDU) as exc_info:
        backend.exchange(cla=CLA, ins=InsType.SET_ACTION,
                         p1=0x01, p2=0x01, data=two_zero)
    assert exc_info.value.status == StatusWords.SWO_INCORRECT_P1_P2

    # SIGN_ACTION: expects P1=0x00, P2=0x00
    with pytest.raises(ExceptionRAPDU) as exc_info:
        backend.exchange(cla=CLA, ins=InsType.SIGN_ACTION,
                         p1=0x01, p2=0x00, data=path_data)
    assert exc_info.value.status == StatusWords.SWO_INCORRECT_P1_P2


# ── Wrong-sequencing validation ───────────────────────────────────────────────

def test_sign_action_without_metadata(backend: BackendInterface) -> None:
    """SIGN_ACTION before PROVIDE_ACTION_METADATA must be rejected with SWO_INCORRECT_DATA.

    When no metadata is set, handle_ui() receives a NULL pointer and returns false,
    causing handler_sign_action() to return SWO_INCORRECT_DATA.
    """
    with pytest.raises(ExceptionRAPDU) as exc_info:
        backend.exchange(cla=CLA, ins=InsType.SIGN_ACTION,
                         p1=0x00, p2=0x00,
                         data=pack_derivation_path("m/44'/60'/0'/0/0"))
    assert exc_info.value.status == StatusWords.SWO_INCORRECT_DATA


# ── Metadata integrity validation ─────────────────────────────────────────────

def test_error_bad_metadata_signature(backend: BackendInterface) -> None:
    """Metadata with a tampered PKI signature must be rejected with SWO_INCORRECT_DATA.

    ActionMetadata accepts an explicit signature parameter; passing an obviously wrong
    DER sequence exercises the verify_action_metadata() PKI check path.
    """
    client = CommandSender(backend)
    bad_sig = bytes(73)
    with pytest.raises(ExceptionRAPDU) as exc_info:
        client.provide_action_metadata(ActionMetadata(
            version=1,
            operation_type=OperationType.ORDER,
            asset_id=1,
            asset_ticker="ETH",
            network=Network.MAINNET,
            signature=bad_sig,
        ))
    assert exc_info.value.status == StatusWords.SWO_INCORRECT_DATA
