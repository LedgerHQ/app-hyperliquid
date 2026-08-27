import pytest
from application_client.action_metadata import ActionMetadata, Network, OperationType
from application_client.bulk_modify import BulkModify, ModifyRequest
from application_client.bulk_order import BuilderInfo, BulkOrder, Grouping
from application_client.command_sender import CommandSender
from application_client.order_request import Limit, OrderRequest, OrderType, Tif, Trigger, TriggerType
from application_client.set_action import ActionType, SetAction
from ragger.error import ExceptionRAPDU, StatusWords
from ragger.navigator.navigation_scenario import NavigateWithScenario

_BUILDER = BuilderInfo(
    bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
    100,
)


def _send_order_setup(client: CommandSender, orders: list) -> None:
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.ORDER,
        1,
        "ETH",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.BULK_ORDER,
        1772544778964,
        BulkOrder(list(orders), Grouping.NORMAL_TPSL, _BUILDER),
    ))


def _honest_limit() -> OrderRequest:
    return OrderRequest(
        OrderType.LIMIT,
        1,
        False,
        "2200",
        "0.5128",
        False,
        Limit(Tif.GTC),
    )


def _honest_trigger(tpsl: TriggerType) -> OrderRequest:
    px = "500" if tpsl == TriggerType.TP else "2500"
    return OrderRequest(
        OrderType.TRIGGER,
        1,
        True,
        px,
        "0.5128",
        True,
        Trigger(True, px, tpsl),
    )


def _expect_rejected(client: CommandSender) -> None:
    with pytest.raises(ExceptionRAPDU) as exc_info, client.sign_action("m/44'/60'/0'/0/0"):
        pass
    assert exc_info.value.status == StatusWords.SWO_INCORRECT_DATA


def test_sign_order_tp_not_reduce_only_rejected(scenario_navigator: NavigateWithScenario) -> None:
    """TP trigger with reduce_only=False can open a position while shown as a TP price."""
    client = CommandSender(scenario_navigator.backend)
    tp = _honest_trigger(TriggerType.TP)
    tp.reduce_only = False
    _send_order_setup(client, [_honest_limit(), tp])
    _expect_rejected(client)


def test_sign_order_tp_wrong_side_rejected(scenario_navigator: NavigateWithScenario) -> None:
    """TP trigger on the same side as the parent order increases exposure instead of closing."""
    client = CommandSender(scenario_navigator.backend)
    tp = _honest_trigger(TriggerType.TP)
    tp.is_buy = False
    _send_order_setup(client, [_honest_limit(), tp])
    _expect_rejected(client)


def test_sign_order_tp_size_mismatch_rejected(scenario_navigator: NavigateWithScenario) -> None:
    """TP trigger larger than the displayed size closes more than reviewed."""
    client = CommandSender(scenario_navigator.backend)
    tp = _honest_trigger(TriggerType.TP)
    tp.sz = "10"
    _send_order_setup(client, [_honest_limit(), tp])
    _expect_rejected(client)


def test_sign_order_trigger_limit_mode_rejected(scenario_navigator: NavigateWithScenario) -> None:
    """Trigger with is_market=False and a hidden limit price deviates from the reviewed price."""
    client = CommandSender(scenario_navigator.backend)
    tp = _honest_trigger(TriggerType.TP)
    tp.order.is_market = False
    _send_order_setup(client, [_honest_limit(), tp])
    _expect_rejected(client)


def test_sign_order_trigger_hidden_limit_px_rejected(scenario_navigator: NavigateWithScenario) -> None:
    """Trigger whose signed limit_px differs from the displayed trigger price."""
    client = CommandSender(scenario_navigator.backend)
    tp = _honest_trigger(TriggerType.TP)
    tp.limit_px = "1"
    _send_order_setup(client, [_honest_limit(), tp])
    _expect_rejected(client)


def test_sign_set_tp_sl_size_mismatch_rejected(scenario_navigator: NavigateWithScenario) -> None:
    """TP/SL pair sharing one displayed size must carry the same signed size."""
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.MODIFY,
        1,
        "ETH",
        Network.MAINNET))
    tp = _honest_trigger(TriggerType.TP)
    tp.is_buy = False
    tp.limit_px = "2276.3"
    tp.sz = "0.0072"
    tp.order = Trigger(True, "2276.3", TriggerType.TP)
    sl = _honest_trigger(TriggerType.SL)
    sl.is_buy = False
    sl.limit_px = "1965.9"
    sl.sz = "10"
    sl.order = Trigger(True, "1965.9", TriggerType.SL)
    client.set_action(SetAction(
        1,
        ActionType.BULK_ORDER,
        1773335540229,
        BulkOrder([tp, sl], Grouping.POSITION_TPSL, _BUILDER),
    ))
    _expect_rejected(client)


def test_sign_modify_not_reduce_only_rejected(scenario_navigator: NavigateWithScenario) -> None:
    """Modified TP with reduce_only=False can redirect into an opening order."""
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.MODIFY,
        0,
        "BTC",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.BULK_MODIFY,
        1773050015814,
        BulkModify([
            ModifyRequest(
                OrderRequest(
                    OrderType.TRIGGER,
                    0,
                    False,
                    "85169",
                    "0.0005",
                    False,
                    Trigger(True, "85169", TriggerType.TP),
                ),
                343050796655,
            ),
        ]),
    ))
    _expect_rejected(client)
