from application_client.action_metadata import ActionMetadata, Network, OperationType
from application_client.approve_builder_fee import ApproveBuilderFee
from application_client.bulk_cancel import BulkCancel, CancelRequest
from application_client.bulk_modify import BulkModify, ModifyRequest
from application_client.bulk_order import BuilderInfo, BulkOrder, Grouping
from application_client.command_sender import CommandSender
from application_client.order_request import Limit, OrderRequest, OrderType, Tif, Trigger, TriggerType
from application_client.response_unpacker import unpack_sign_action_response
from application_client.set_action import ActionType, SetAction
from application_client.update_isolated_margin import UpdateIsolatedMargin
from application_client.update_leverage import UpdateLeverage
from ragger.navigator.navigation_scenario import NavigateWithScenario


def common_sign(client: CommandSender,
                scenario_navigator: NavigateWithScenario,
                path: str = "m/44'/60'/0'/0/0") -> None:
    with client.sign_action(path):
        scenario_navigator.review_approve(custom_screen_text="Sign message to", do_comparison=True)
    remaining, _, _, _ = unpack_sign_action_response(client.backend.last_async_response.data)
    while remaining > 0:
        with client.sign_action(path):
            pass
        remaining, _, _, _ = unpack_sign_action_response(client.backend.last_async_response.data)

def test_sign_market_order(scenario_navigator: NavigateWithScenario) -> None:
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.ORDER,
        1,
        "ETH",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.APPROVAL_BUILDER_FEE,
        1772546706895,
        ApproveBuilderFee(
            42161,
            "0.1000%",
            bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
        ),
    ))
    client.set_action(SetAction(
        1,
        ActionType.UPDATE_LEVERAGE,
        1772544778963,
        UpdateLeverage(
            1,
            False,
            10,
        ),
    ))
    client.set_action(SetAction(
        1,
        ActionType.BULK_ORDER,
        1772544778964,
        BulkOrder(
            [
                OrderRequest(
                    OrderType.LIMIT,
                    1,
                    True,
                    "1996.7",
                    "0.5108",
                    False,
                    Limit(Tif.IOC),
                ),
            ],
            Grouping.NA,
            BuilderInfo(
                bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
                100,
            ),
        ),
    ))
    common_sign(client, scenario_navigator)

def test_sign_limit_order(scenario_navigator: NavigateWithScenario) -> None:
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.ORDER,
        1,
        "ETH",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.APPROVAL_BUILDER_FEE,
        1772544778962,
        ApproveBuilderFee(
            42161,
            "0.1000%",
            bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
        ),
    ))
    client.set_action(SetAction(
        1,
        ActionType.UPDATE_LEVERAGE,
        1772544778963,
        UpdateLeverage(
            1,
            False,
            10,
        ),
    ))
    client.set_action(SetAction(
        1,
        ActionType.BULK_ORDER,
        1772544778964,
        BulkOrder(
            [
                OrderRequest(
                    OrderType.LIMIT,
                    1,
                    False,
                    "2200",
                    "0.5128",
                    False,
                    Limit(Tif.GTC),
                ),
                OrderRequest(
                    OrderType.TRIGGER,
                    1,
                    True,
                    "500",
                    "0.5128",
                    True,
                    Trigger(
                        True,
                        "500",
                        TriggerType.TP,
                    ),
                ),
                OrderRequest(
                    OrderType.TRIGGER,
                    1,
                    True,
                    "2500",
                    "0.5128",
                    True,
                    Trigger(
                        True,
                        "2500",
                        TriggerType.SL,
                    ),
                ),
            ],
            Grouping.NORMAL_TPSL,
            BuilderInfo(
                bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
                100,
            ),
        ),
    ))
    common_sign(client, scenario_navigator)

def test_sign_edit(scenario_navigator: NavigateWithScenario) -> None:
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
                    True,
                    Trigger(True, "85169", TriggerType.TP),
                ),
                343050796655,
            ),
        ]),
    ))
    common_sign(client, scenario_navigator)

def test_sign_set_tp_sl(scenario_navigator: NavigateWithScenario) -> None:
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.MODIFY,
        1,
        "ETH",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.BULK_ORDER,
        1773335540229,
        BulkOrder(
            [
                OrderRequest(
                    OrderType.TRIGGER,
                    1,
                    False,
                    "2276.3",
                    "0.0072",
                    True,
                    Trigger(
                        True,
                        "2276.3",
                        TriggerType.TP,
                    ),
                ),
                OrderRequest(
                    OrderType.TRIGGER,
                    1,
                    True,
                    "1965.9",
                    "0.0072",
                    True,
                    Trigger(
                        True,
                        "1965.9",
                        TriggerType.SL,
                    ),
                ),
            ],
            Grouping.POSITION_TPSL,
            BuilderInfo(
                bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
                100,
            ),
        ),
    ))
    common_sign(client, scenario_navigator)

def test_sign_cancel(scenario_navigator: NavigateWithScenario) -> None:
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.CANCEL,
        0,
        "BTC",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.BULK_CANCEL,
        1772813983827,
        BulkCancel([
            CancelRequest(0, 340574409238),
        ]),
    ))
    common_sign(client, scenario_navigator)

def test_sign_close(scenario_navigator: NavigateWithScenario) -> None:
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.CLOSE,
        0,
        "BTC",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.BULK_ORDER,
        1772814377109,
        BulkOrder(
            [
                OrderRequest(
                    OrderType.LIMIT,
                    0,
                    True,
                    "69792",
                    "0.00021",
                    True,
                    Limit(Tif.IOC),
                ),
            ],
            Grouping.NA,
            BuilderInfo(
                bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
                100,
            ),
        ),
    ))
    common_sign(client, scenario_navigator)

def test_sign_add_margin(scenario_navigator: NavigateWithScenario) -> None:
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.UPDATE_MARGIN,
        0,
        "BTC",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.UPDATE_ISOLATED_MARGIN,
        1773141130938,
        UpdateIsolatedMargin(
            0,
            True,
            4200000,
        ),
    ))
    common_sign(client, scenario_navigator)

def test_sign_remove_margin(scenario_navigator: NavigateWithScenario) -> None:
    client = CommandSender(scenario_navigator.backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        OperationType.UPDATE_MARGIN,
        0,
        "BTC",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.UPDATE_ISOLATED_MARGIN,
        1773141130952,
        UpdateIsolatedMargin(
            0,
            True,
            -2690000,
        ),
    ))
    common_sign(client, scenario_navigator)
