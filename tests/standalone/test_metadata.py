from application_client.action_metadata import ActionMetadata, ActionType, Network
from application_client.command_sender import CommandSender
from ragger.backend.interface import BackendInterface


def test_action_metadata(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.provide_action_metadata(ActionMetadata(
        1,
        ActionType.ORDER,
        42,
        "BTC",
        Network.MAINNET))
