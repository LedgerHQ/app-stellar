from ledgered.devices import DeviceType
from ragger.navigator import NavInsID
from utils import ROOT_SCREENSHOT_PATH


def test_app_mainmenu(device, navigator, test_name):
    if device.is_nano:
        instructions = [
            NavInsID.RIGHT_CLICK,
            NavInsID.BOTH_CLICK,
            NavInsID.RIGHT_CLICK,
            NavInsID.RIGHT_CLICK,
            NavInsID.RIGHT_CLICK,
            NavInsID.RIGHT_CLICK,
            NavInsID.RIGHT_CLICK,
            NavInsID.BOTH_CLICK,
        ]
    else:
        if device.type == DeviceType.STAX:
            instructions = [
                NavInsID.USE_CASE_HOME_SETTINGS,
                NavInsID.USE_CASE_SUB_SETTINGS_NEXT,
                NavInsID.USE_CASE_SUB_SETTINGS_EXIT,
            ]
        else:
            instructions = [
                NavInsID.USE_CASE_HOME_SETTINGS,
                NavInsID.USE_CASE_SUB_SETTINGS_NEXT,
                NavInsID.USE_CASE_SUB_SETTINGS_NEXT,
                NavInsID.USE_CASE_SUB_SETTINGS_EXIT,
            ]
    navigator.navigate_and_compare(
        ROOT_SCREENSHOT_PATH,
        test_name,
        instructions,
        screen_change_before_first_instruction=False,
    )
