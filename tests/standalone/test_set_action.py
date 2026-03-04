from application_client.approve_builder_fee import ApproveBuilderFee
from application_client.bulk_cancel import BulkCancel, CancelRequest
from application_client.bulk_modify import BulkModify, ModifyRequest
from application_client.bulk_order import BuilderInfo, BulkOrder, Grouping
from application_client.command_sender import CommandSender
from application_client.order_request import Limit, OrderRequest, OrderType, Tif
from application_client.set_action import ActionType, SetAction
from application_client.update_leverage import UpdateLeverage
from ragger.backend.interface import BackendInterface


def test_set_action_bulk_order(backend: BackendInterface) -> None:
    client = CommandSender(backend)
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
            ],
            Grouping.NA,
            BuilderInfo(
                bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
                100,
            ),
        ),
    ))

def test_set_action_bulk_modify(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.set_action(SetAction(
        1,
        ActionType.MODIFY,
        1770816625873,
        BulkModify([
            ModifyRequest(
                OrderRequest(
                    OrderType.LIMIT,
                    1,
                    True,
                    "1992",
                    "0.512",
                    False,
                    Limit(Tif.IOC),
                ),
                42,
            ),
            ModifyRequest(
                OrderRequest(
                    OrderType.LIMIT,
                    2,
                    True,
                    "254",
                    "2.3",
                    False,
                    Limit(Tif.ALO),
                ),
                21,
            ),
        ]),
    ))

def test_set_action_bulk_cancel(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.set_action(SetAction(
        1,
        ActionType.CANCEL,
        1770816625873,
        BulkCancel([
            CancelRequest(1, 42),
            CancelRequest(2, 21),
        ]),
    ))

def test_set_action_update_leverage(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.set_action(SetAction(
        1,
        ActionType.UPDATE_LEVERAGE,
        1770816625873,
        UpdateLeverage(
            1,
            False,
            8,
        ),
    ))

def test_set_action_approval_builder_fee(backend: BackendInterface) -> None:
    client = CommandSender(backend)
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
