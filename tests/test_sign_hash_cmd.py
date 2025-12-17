import pytest
from application_client.stellar_command_sender import Errors, StellarCommandSender
from ragger.error import ExceptionRAPDU
from stellar_sdk import Keypair
from stellar_sdk.utils import sha256
from utils import (
    ROOT_SCREENSHOT_PATH,
    SettingsId,
    configure_device_settings,
    handle_risk_warning,
)

from dataset import MNEMONIC


def test_sign_hash(backend, scenario_navigator, device, navigator):
    keypair = Keypair.from_mnemonic_phrase(MNEMONIC, index=0)
    path = "m/44'/148'/0'"
    client = StellarCommandSender(backend)

    configure_device_settings(navigator, device, SettingsId.ENABLE_BLIND_SIGNING)

    hash_to_sign = sha256(b"Hello, Ledger & Stellar!")
    with client.sign_hash(path=path, hash_to_sign=hash_to_sign):
        handle_risk_warning(navigator, device)
        scenario_navigator.review_approve(
            ROOT_SCREENSHOT_PATH,
            custom_screen_text="Sign ",
        )

    response = client.get_async_response().data

    expected_signature = keypair.sign(hash_to_sign)
    assert response == expected_signature


def test_sign_hash_reject_blind_signing(backend, device, navigator):
    path = "m/44'/148'/0'"
    client = StellarCommandSender(backend)

    configure_device_settings(navigator, device, SettingsId.ENABLE_BLIND_SIGNING)

    hash_to_sign = sha256(b"Hello, Ledger & Stellar!")

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_hash(path=path, hash_to_sign=hash_to_sign):
            handle_risk_warning(navigator, device, accept=False)

    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0


def test_sign_hash_reject_sign(backend, scenario_navigator, device, navigator):
    path = "m/44'/148'/0'"
    client = StellarCommandSender(backend)

    configure_device_settings(navigator, device, SettingsId.ENABLE_BLIND_SIGNING)

    hash_to_sign = sha256(b"Hello, Ledger & Stellar!")

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_hash(path=path, hash_to_sign=hash_to_sign):
            handle_risk_warning(navigator, device)
            scenario_navigator.review_reject(ROOT_SCREENSHOT_PATH)

    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0


@pytest.mark.parametrize(
    "invalid_hash",
    [
        b"",
        b"\x00" * 31,
        b"\x00" * 33,
    ],
    ids=[
        "empty-hash",
        "too-short-hash",
        "too-long-hash",
    ],
)
def test_sign_hash_with_invalid_hash(backend, device, navigator, invalid_hash):
    path = "m/44'/148'/0'"
    client = StellarCommandSender(backend)

    configure_device_settings(navigator, device, SettingsId.ENABLE_BLIND_SIGNING)

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_hash(path=path, hash_to_sign=invalid_hash):
            pass

    assert e.value.status == Errors.SW_WRONG_APDU_LENGTH
    assert len(e.value.data) == 0
