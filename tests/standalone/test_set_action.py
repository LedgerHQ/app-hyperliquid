from application_client.bulk_cancel import BulkCancel
from application_client.bulk_modify import BulkModify
from application_client.bulk_order import BulkOrder, Grouping
from application_client.command_sender import CommandSender
from application_client.order import Order, OrderType, Tif
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
            Order(
                OrderType.LIMIT,
                1,
                True,
                "1992",
                "0.512",
                False,
                Tif.IOC,
            ),
            Grouping.NA,
            builder_addr=bytes.fromhex("c0708cdd6cd166d51da264e3f49a0422be26e35b"),
            builder_fee=100,
        ),
    ))

def test_set_action_bulk_modify(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.set_action(SetAction(
        1,
        ActionType.MODIFY,
        1770816625873,
        BulkModify(
            Order(
                OrderType.LIMIT,
                1,
                True,
                "1992",
                "0.512",
                False,
                Tif.IOC,
            ),
            42,
        ),
    ))

def test_set_action_bulk_cancel(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.set_action(SetAction(
        1,
        ActionType.CANCEL,
        1770816625873,
        BulkCancel(
            1,
            42,
        ),
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
