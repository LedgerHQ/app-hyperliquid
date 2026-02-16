from application_client.bulk_order import BulkOrder, Grouping
from application_client.command_sender import CommandSender
from application_client.order import Order, OrderType, Tif
from application_client.set_action import ActionType, SetAction
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
