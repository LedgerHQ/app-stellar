import pytest
from application_client.stellar_command_sender import Errors, StellarCommandSender
from ragger.error import ExceptionRAPDU
from stellar_sdk import Keypair, StrKey
from utils import ROOT_SCREENSHOT_PATH

from dataset import MNEMONIC


def get_index_from_path(path: str) -> int:
    """Extract the index from a BIP32 path string."""
    return int(path.split("/")[-1].rstrip("'"))


@pytest.mark.parametrize(
    "path",
    [
        "m/44'/148'/0'",
        "m/44'/148'/1'",
        "m/44'/148'/2'",
        "m/44'/148'/255'",
        "m/44'/148'/2147483647'",
    ],
    ids=[
        "account-index-0",
        "account-index-1",
        "account-index-2",
        "account-index-255",
        "account-index-max",
    ],
)
def test_get_public_key_no_confirm(backend, path):
    client = StellarCommandSender(backend)
    response = client.get_public_key(path=path).data
    public_key = StrKey.encode_ed25519_public_key(response)

    index = get_index_from_path(path)
    expected_public_key = Keypair.from_mnemonic_phrase(MNEMONIC, index=index).public_key
    assert public_key == expected_public_key


def test_get_public_key_confirm_accepted(backend, scenario_navigator):
    client = StellarCommandSender(backend)
    path = "m/44'/148'/255'"

    with client.get_public_key_with_confirmation(path=path):
        scenario_navigator.address_review_approve(ROOT_SCREENSHOT_PATH)

    response = client.get_async_response().data
    public_key = StrKey.encode_ed25519_public_key(response)
    index = get_index_from_path(path)
    expected_public_key = Keypair.from_mnemonic_phrase(MNEMONIC, index=index).public_key
    assert public_key == expected_public_key


def test_get_public_key_confirm_refused(backend, scenario_navigator):
    client = StellarCommandSender(backend)
    path = "m/44'/148'/255'"

    with pytest.raises(ExceptionRAPDU) as e:
        with client.get_public_key_with_confirmation(path=path):
            scenario_navigator.address_review_reject(ROOT_SCREENSHOT_PATH)

    # Assert that we have received a refusal
    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0


def test_get_public_key_invalid_path_depth(backend):
    client = StellarCommandSender(backend)
    path = "m/44'/148'/0'/1"

    with pytest.raises(ExceptionRAPDU) as e:
        client.get_public_key(path=path)

    assert e.value.status == Errors.SW_WRONG_APDU_LENGTH
