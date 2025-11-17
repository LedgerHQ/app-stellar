import os

import pytest
from application_client.stellar_command_sender import Errors, StellarCommandSender
from ragger.error import ExceptionRAPDU
from stellar_sdk import Keypair
from stellar_sdk.utils import sha256
from utils import (
    ROOT_SCREENSHOT_PATH,
    SettingsId,
    configure_device_settings,
    get_testcases_names,
    handle_risk_warning,
)

from dataset import MNEMONIC, SignTxTestCases


@pytest.mark.parametrize("test_name", get_testcases_names(SignTxTestCases))
def test_sign_tx(backend, scenario_navigator, navigator, device, test_name):
    keypair = Keypair.from_mnemonic_phrase(MNEMONIC, index=0)
    path = "m/44'/148'/0'"
    transaction = getattr(SignTxTestCases, test_name)()
    client = StellarCommandSender(backend)
    signature_base = transaction.signature_base()

    if "op_invoke_host_function" in test_name:
        # For host function transactions, enable blind signing
        configure_device_settings(
            navigator,
            device,
            SettingsId.ENABLE_BLIND_SIGNING,
        )

    # Send the sign device instruction.
    # As it requires on-screen validation, the function is asynchronous.
    # It will yield the result when the navigation is done
    with client.sign_tx(path=path, transaction=signature_base):
        if "op_invoke_host_function" in test_name:
            # For host function transactions, handle the risk warning
            handle_risk_warning(navigator, device)
        # Validate the on-screen request by performing the navigation appropriate for this device
        scenario_navigator.review_approve(
            ROOT_SCREENSHOT_PATH,
            test_name=f"test_sign_tx_{test_name}",
            custom_screen_text="Sign ",
        )
    response = client.get_async_response().data

    expected_signature = keypair.sign(sha256(transaction.signature_base()))
    assert response == expected_signature


@pytest.mark.parametrize(
    "test_name", get_testcases_names(SignTxTestCases, filter="cond_")
)
def test_sign_tx_with_precondition_enabled(
    backend, scenario_navigator, device, navigator, test_name
):
    keypair = Keypair.from_mnemonic_phrase(MNEMONIC, index=0)
    path = "m/44'/148'/0'"
    transaction = getattr(SignTxTestCases, test_name)()
    client = StellarCommandSender(backend)
    signature_base = transaction.signature_base()

    configure_device_settings(navigator, device, SettingsId.ENABLE_PRECONDITION)

    with client.sign_tx(path=path, transaction=signature_base):
        scenario_navigator.review_approve(
            ROOT_SCREENSHOT_PATH,
            test_name=f"test_sign_tx_with_precondition_enabled_{test_name}",
            custom_screen_text="Sign ",
        )
    response = client.get_async_response().data

    expected_signature = keypair.sign(sha256(transaction.signature_base()))
    assert response == expected_signature


def test_sign_tx_with_sequence_enabled(backend, scenario_navigator, device, navigator):
    keypair = Keypair.from_mnemonic_phrase(MNEMONIC, index=0)
    path = "m/44'/148'/0'"
    transaction = SignTxTestCases.op_payment_asset_native()
    client = StellarCommandSender(backend)
    signature_base = transaction.signature_base()

    configure_device_settings(navigator, device, SettingsId.ENABLE_SEQUENCE_AND_NONCE)

    with client.sign_tx(path=path, transaction=signature_base):
        scenario_navigator.review_approve(
            ROOT_SCREENSHOT_PATH,
            custom_screen_text="Sign ",
        )
    response = client.get_async_response().data

    expected_signature = keypair.sign(sha256(transaction.signature_base()))
    assert response == expected_signature


def test_sign_tx_with_tx_source_enabled(backend, scenario_navigator, device, navigator):
    keypair = Keypair.from_mnemonic_phrase(MNEMONIC, index=1)
    path = "m/44'/148'/1'"
    transaction = SignTxTestCases.op_payment_asset_native()
    client = StellarCommandSender(backend)
    signature_base = transaction.signature_base()

    configure_device_settings(navigator, device, SettingsId.ENABLE_TRANSACTION_SOURCE)

    with client.sign_tx(path=path, transaction=signature_base):
        scenario_navigator.review_approve(
            ROOT_SCREENSHOT_PATH,
            custom_screen_text="Sign ",
        )
    response = client.get_async_response().data

    expected_signature = keypair.sign(sha256(transaction.signature_base()))
    assert response == expected_signature


def test_sign_tx_with_nested_authorization_disabled(
    backend, scenario_navigator, device, navigator
):
    keypair = Keypair.from_mnemonic_phrase(MNEMONIC, index=0)
    path = "m/44'/148'/0'"
    transaction = SignTxTestCases.op_invoke_host_function_with_complex_sub_invocation()
    client = StellarCommandSender(backend)
    signature_base = transaction.signature_base()
    configure_device_settings(
        navigator,
        device,
        SettingsId.DISABLE_NESTED_AUTHORIZATION | SettingsId.ENABLE_BLIND_SIGNING,
    )

    with client.sign_tx(path=path, transaction=signature_base):
        handle_risk_warning(navigator, device)
        scenario_navigator.review_approve(
            ROOT_SCREENSHOT_PATH,
            custom_screen_text="Sign ",
        )
    response = client.get_async_response().data

    expected_signature = keypair.sign(sha256(transaction.signature_base()))
    assert response == expected_signature


def test_sign_tx_reject(backend, scenario_navigator):
    path = "m/44'/148'/0'"
    transaction = SignTxTestCases.op_payment_asset_native()
    client = StellarCommandSender(backend)

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_tx(path=path, transaction=transaction.signature_base()):
            scenario_navigator.review_reject(ROOT_SCREENSHOT_PATH)

    # Assert that we have received a refusal
    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0


def test_sign_tx_data_too_large(backend):
    path = "m/44'/148'/0'"
    transaction = os.urandom(1024 * 10)  # 10 KB transaction
    client = StellarCommandSender(backend)

    with pytest.raises(ExceptionRAPDU) as e:
        with client.sign_tx(path=path, transaction=transaction):
            pass

    assert e.value.status == Errors.SW_REQUEST_DATA_TOO_LARGE
    assert len(e.value.data) == 0
