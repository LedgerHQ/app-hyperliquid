from application_client.action_metadata import ActionMetadata, ActionType, Network
from application_client.approve_builder_fee import ApproveBuilderFee
from application_client.bulk_order import BuilderInfo, BulkOrder, Grouping
from application_client.command_sender import CommandSender
from application_client.order_request import Limit, OrderRequest, OrderType, Tif, Trigger, TriggerType
from application_client.set_action import SetAction
from application_client.update_leverage import UpdateLeverage
from ragger.backend.interface import BackendInterface


def test_sign_action(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        ActionType.ORDER,
        42,
        "BTC",
        Network.MAINNET))
    client.set_action(SetAction(
        1,
        ActionType.ORDER,
        1770816625873,
        BulkOrder(
            [
                OrderRequest(
                    OrderType.LIMIT,
                    1,
                    True,
                    "1992",
                    "0.512",
                    False,
                    Limit(Tif.IOC),
                ),
                OrderRequest(
                    OrderType.TRIGGER,
                    1,
                    True,
                    "1992",
                    "0.512",
                    False,
                    Trigger(
                        True,
                        "190",
                        TriggerType.TP,
                    ),
                ),
            ],
            Grouping.NA,
            BuilderInfo(
                bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
                100,
            ),
        ),
    ))
    client.set_action(SetAction(
        1,
        ActionType.UPDATE_LEVERAGE,
        1770816626930,
        UpdateLeverage(
            1,
            False,
            8,
        ),
    ))
    client.set_action(SetAction(
        1,
        ActionType.APPROVAL_BUILDER_FEE,
        1770816625903,
        ApproveBuilderFee(
            42161,
            "0.15%",
            bytes.fromhex("d8dA6BF26964aF9D7eEd9e03E53415D37aA96045"),
        ),
    ))
    client.sign_action("m/44'/60'/0'/0/0")
    client.sign_action("m/44'/60'/0'/0/1")
    client.sign_action("m/44'/60'/0'/0/1")
