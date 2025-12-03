import pytest
from application_client.stellar_command_sender import Errors, StellarCommandSender
from ragger.error import ExceptionRAPDU
from stellar_sdk import Keypair
from utils import ROOT_SCREENSHOT_PATH

from dataset import MNEMONIC


@pytest.mark.parametrize(
    "_id, message",
    [
        ("simple_message", b"Hello, Ledger & Stellar!"),
        ("long_message", b"-".join(str(i).zfill(3).encode() for i in range(1, 501))),
        (
            "unprintable_bytes_message",
            b"Hello\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f",
        ),
    ],
    ids=[
        "simple_message",
        "long_message",
        "unprintable_bytes_message",
    ],
)
def test_sign_message(backend, scenario_navigator, test_name, message, _id):
    keypair = Keypair.from_mnemonic_phrase(MNEMONIC, index=0)
    path = "m/44'/148'/0'"
    client = StellarCommandSender(backend)

    with client.sign_message(path=path, message=message):
        scenario_navigator.review_approve(
            ROOT_SCREENSHOT_PATH,
            test_name=f"{test_name}_{_id}",
            custom_screen_text="Sign ",
        )

    response = client.get_async_response().data

    expected_signature = keypair.sign_message(message)
    assert response == expected_signature


def test_sign_message_reject(backend, scenario_navigator):
    path = "m/44'/148'/0'"
    client = StellarCommandSender(backend)
    message = b"Hello, Ledger & Stellar!"

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_message(path=path, message=message):
            scenario_navigator.review_reject(ROOT_SCREENSHOT_PATH)

    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0


def test_sign_message_data_too_large(backend):
    path = "m/44'/148'/0'"
    client = StellarCommandSender(backend)
    message = b"A" * 1024 * 10

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_message(path=path, message=message):
            pass

    assert e.value.status == Errors.SW_REQUEST_DATA_TOO_LARGE
