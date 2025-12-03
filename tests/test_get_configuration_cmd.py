from application_client.stellar_command_sender import StellarCommandSender
from ledgered.devices import DeviceType
from utils import SettingsId, configure_device_settings


def test_get_configuration(backend, device):
    client = StellarCommandSender(backend)
    response = client.get_configuration().data
    version, blind_signing_enabled, max_data_size = unpack_response(response)
    assert version.count(".") == 2
    assert blind_signing_enabled is False
    assert max_data_size == 1024 * 4 if device.type == DeviceType.NANOX else 1024 * 8


def test_get_configuration_with_blind_signing_enabled(backend, navigator, device):
    client = StellarCommandSender(backend)
    configure_device_settings(navigator, device, SettingsId.ENABLE_BLIND_SIGNING)
    response = client.get_configuration().data
    version, blind_signing_enabled, max_data_size = unpack_response(response)
    assert version.count(".") == 2
    assert blind_signing_enabled is True
    assert max_data_size == 1024 * 4 if device.type == DeviceType.NANOX else 1024 * 8


def unpack_response(response) -> tuple[str, bool, int | None]:
    blind_signing_enabled: bool = response[0] == 0x01
    major: int = response[1]
    minor: int = response[2]
    patch: int = response[3]
    version: str = f"{major}.{minor}.{patch}"
    max_data_size: int = response[4] << 8 | response[5]
    return version, blind_signing_enabled, max_data_size
