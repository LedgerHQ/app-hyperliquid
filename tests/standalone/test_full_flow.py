from application_client.action_metadata import ActionMetadata, Network, OperationType
from application_client.approve_builder_fee import ApproveBuilderFee
from application_client.bulk_order import BuilderInfo, BulkOrder, Grouping
from application_client.command_sender import CommandSender
from application_client.order_request import Limit, OrderRequest, OrderType, Tif
from application_client.set_action import ActionType, SetAction
from application_client.update_leverage import UpdateLeverage
from ragger.backend.interface import BackendInterface


def test_sign_market_order(backend: BackendInterface) -> None:
    client = CommandSender(backend)
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
    client.sign_action("m/44'/60'/0'/0/0")
    client.sign_action("m/44'/60'/0'/0/0")
    client.sign_action("m/44'/60'/0'/0/0")
