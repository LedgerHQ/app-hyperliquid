from application_client.command_sender import CommandSender
from ragger.backend.interface import BackendInterface


def test_get_address(backend: BackendInterface) -> None:
    client = CommandSender(backend)
    client.get_address("m/44'/60'/0'/0/0")
