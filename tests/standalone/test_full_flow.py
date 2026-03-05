from application_client.command_sender import CommandSender
from application_client.response_unpacker import unpack_sign_action_response
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
