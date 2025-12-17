from enum import IntFlag, auto
from pathlib import Path

from ledgered.devices import DeviceType
from ragger.navigator import NavIns, NavInsID

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()


def get_testcases_names(class_type: type, filter: str | None = None) -> list[str]:
    """Get test case names from a class by filtering staticmethod names."""
    return [
        name
        for name, function in vars(class_type).items()
        if isinstance(function, staticmethod) and (filter is None or filter in name)
    ]


class SettingsId(IntFlag):
    """Settings flags for device configuration."""

    ENABLE_BLIND_SIGNING = auto()
    ENABLE_TRANSACTION_SOURCE = auto()
    ENABLE_SEQUENCE_AND_NONCE = auto()
    ENABLE_PRECONDITION = auto()
    DISABLE_NESTED_AUTHORIZATION = auto()


# Button coordinates for different device types
_SETTINGS_BUTTONS = {
    DeviceType.STAX: {
        SettingsId.ENABLE_BLIND_SIGNING: (340, 140),
        SettingsId.ENABLE_TRANSACTION_SOURCE: (340, 280),
        SettingsId.ENABLE_SEQUENCE_AND_NONCE: (340, 445),
        SettingsId.ENABLE_PRECONDITION: (340, 135),
        SettingsId.DISABLE_NESTED_AUTHORIZATION: (340, 305),
    },
    DeviceType.FLEX: {
        SettingsId.ENABLE_BLIND_SIGNING: (415, 140),
        SettingsId.ENABLE_TRANSACTION_SOURCE: (415, 280),
        SettingsId.ENABLE_SEQUENCE_AND_NONCE: (415, 140),
        SettingsId.ENABLE_PRECONDITION: (415, 320),
        SettingsId.DISABLE_NESTED_AUTHORIZATION: (415, 140),
    },
    DeviceType.APEX_P: {
        SettingsId.ENABLE_BLIND_SIGNING: (265, 95),
        SettingsId.ENABLE_TRANSACTION_SOURCE: (265, 190),
        SettingsId.ENABLE_SEQUENCE_AND_NONCE: (265, 95),
        SettingsId.ENABLE_PRECONDITION: (265, 210),
        SettingsId.DISABLE_NESTED_AUTHORIZATION: (265, 95),
    },
}


def configure_device_settings(navigator, device, settings: SettingsId):
    """Configure device settings by navigating through the settings menu."""
    if device.is_nano:
        instructions = _get_nano_instructions(settings)
    elif device.type in (DeviceType.STAX, DeviceType.FLEX, DeviceType.APEX_P):
        instructions = _get_touchscreen_instructions(device.type, settings)
    else:
        raise NotImplementedError(
            f"Settings navigation not implemented for device: {device}"
        )

    navigator.navigate(
        instructions,
        screen_change_before_first_instruction=False,
        screen_change_after_last_instruction=False,
    )


def _get_nano_instructions(settings: SettingsId) -> list[NavIns]:
    """Generate navigation instructions for Nano devices."""
    instructions = [
        NavIns(NavInsID.RIGHT_CLICK),
        NavIns(NavInsID.BOTH_CLICK),
    ]

    settings_order = [
        SettingsId.ENABLE_BLIND_SIGNING,
        SettingsId.ENABLE_TRANSACTION_SOURCE,
        SettingsId.ENABLE_SEQUENCE_AND_NONCE,
        SettingsId.ENABLE_PRECONDITION,
        SettingsId.DISABLE_NESTED_AUTHORIZATION,
    ]

    for setting in settings_order:
        if setting in settings:
            instructions.append(NavIns(NavInsID.BOTH_CLICK))
        instructions.append(NavIns(NavInsID.RIGHT_CLICK))

    return instructions


# Settings layout for touchscreen devices
_TOUCHSCREEN_LAYOUTS = {
    DeviceType.STAX: [
        # Page 1
        [
            SettingsId.ENABLE_BLIND_SIGNING,
            SettingsId.ENABLE_TRANSACTION_SOURCE,
            SettingsId.ENABLE_SEQUENCE_AND_NONCE,
        ],
        # Page 2
        [
            SettingsId.ENABLE_PRECONDITION,
            SettingsId.DISABLE_NESTED_AUTHORIZATION,
        ],
    ],
    DeviceType.FLEX: [
        # Page 1
        [
            SettingsId.ENABLE_BLIND_SIGNING,
            SettingsId.ENABLE_TRANSACTION_SOURCE,
        ],
        # Page 2
        [
            SettingsId.ENABLE_SEQUENCE_AND_NONCE,
            SettingsId.ENABLE_PRECONDITION,
        ],
        # Page 3
        [
            SettingsId.DISABLE_NESTED_AUTHORIZATION,
        ],
    ],
    DeviceType.APEX_P: [
        # Page 1
        [
            SettingsId.ENABLE_BLIND_SIGNING,
            SettingsId.ENABLE_TRANSACTION_SOURCE,
        ],
        # Page 2
        [
            SettingsId.ENABLE_SEQUENCE_AND_NONCE,
            SettingsId.ENABLE_PRECONDITION,
        ],
        # Page 3
        [
            SettingsId.DISABLE_NESTED_AUTHORIZATION,
        ],
    ],
}


def _get_touchscreen_instructions(
    device_type: DeviceType, settings: SettingsId
) -> list[NavIns | NavInsID]:
    """Generate navigation instructions for touchscreen devices (Stax/Flex/Apex P)."""
    buttons = _SETTINGS_BUTTONS[device_type]
    layout = _TOUCHSCREEN_LAYOUTS[device_type]

    instructions = [NavInsID.USE_CASE_HOME_SETTINGS]

    for page_idx, page_settings in enumerate(layout):
        for setting_id in page_settings:
            if setting_id in settings:
                button_pos = buttons[setting_id]
                instructions.append(NavIns(NavInsID.TOUCH, button_pos))

        # Add navigation to next page, except for the last page
        if page_idx < len(layout) - 1:
            instructions.append(NavInsID.USE_CASE_SUB_SETTINGS_NEXT)

    return instructions


def handle_risk_warning(navigator, device, accept: bool = True) -> None:
    """Handle risk warning prompts on the device."""
    if device.is_nano:
        if accept:
            instructions = [NavIns(NavInsID.BOTH_CLICK)]
        else:
            instructions = [
                NavIns(NavInsID.RIGHT_CLICK),
                NavIns(NavInsID.BOTH_CLICK),
            ]
    else:
        # Note: USE_CASE_CHOICE_REJECT actually accepts, USE_CASE_CHOICE_CONFIRM rejects
        instructions = [
            NavIns(
                NavInsID.USE_CASE_CHOICE_REJECT
                if accept
                else NavInsID.USE_CASE_CHOICE_CONFIRM
            )
        ]

    navigator.navigate(
        instructions,
        screen_change_before_first_instruction=True,
        screen_change_after_last_instruction=False,
    )
