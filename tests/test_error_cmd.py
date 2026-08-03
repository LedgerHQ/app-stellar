import pytest
from application_client.stellar_command_sender import CLA, P1, P2, Errors, InsType
from ragger.bip import pack_derivation_path
from ragger.error import ExceptionRAPDU


# Ensure the app returns an error when a bad CLA is used
def test_bad_cla(backend):
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA + 1, ins=InsType.GET_CONF)
    assert e.value.status == Errors.SW_CLA_NOT_SUPPORTED


# Ensure the app returns an error when a bad INS is used
def test_bad_ins(backend):
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(cla=CLA, ins=0xFF)
    assert e.value.status == Errors.SW_INS_NOT_SUPPORTED


# Ensure the app returns an error when a bad P1 or P2 is used
def test_wrong_p1p2(backend):
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(
            cla=CLA, ins=InsType.GET_CONF, p1=P1.FIRST_APDU + 1, p2=P2.MORE_APDU
        )
    assert e.value.status == Errors.SW_WRONG_P1_P2
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(
            cla=CLA, ins=InsType.GET_CONF, p1=P1.FIRST_APDU, p2=P2.MORE_APDU
        )
    assert e.value.status == Errors.SW_WRONG_P1_P2


# Ensure a continuation chunk without a preceding first chunk is rejected
# instead of proceeding with the default derivation path and empty state
@pytest.mark.parametrize(
    "ins",
    [InsType.SIGN_TX, InsType.SIGN_SOROBAN_AUTH, InsType.SIGN_MESSAGE],
    ids=["sign_tx", "sign_soroban_auth", "sign_message"],
)
def test_continuation_without_first_chunk(backend, ins):
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(
            cla=CLA, ins=ins, p1=P1.MORE_APDU, p2=P2.LAST_APDU, data=b"\x00" * 8
        )
    assert e.value.status == Errors.SW_DATA_PARSING_FAIL


CHUNKED_SIGNING_INS = (
    InsType.SIGN_TX,
    InsType.SIGN_SOROBAN_AUTH,
    InsType.SIGN_MESSAGE,
)


@pytest.mark.parametrize(
    "first_ins,continuation_ins",
    [
        pytest.param(
            first_ins,
            continuation_ins,
            id=f"{first_ins.name.lower()}_to_{continuation_ins.name.lower()}",
        )
        for first_ins in CHUNKED_SIGNING_INS
        for continuation_ins in CHUNKED_SIGNING_INS
        if first_ins != continuation_ins
    ],
)
def test_continuation_with_different_instruction_is_rejected(
    backend, first_ins, continuation_ins
):
    path = pack_derivation_path("m/44'/148'/0'")

    backend.exchange(
        cla=CLA,
        ins=first_ins,
        p1=P1.FIRST_APDU,
        p2=P2.MORE_APDU,
        data=path,
    )

    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(
            cla=CLA,
            ins=continuation_ins,
            p1=P1.MORE_APDU,
            p2=P2.LAST_APDU,
            data=b"\x00" * 8,
        )
    assert e.value.status == Errors.SW_DATA_PARSING_FAIL

    # A mismatched continuation invalidates the original stream as well.
    with pytest.raises(ExceptionRAPDU) as e:
        backend.exchange(
            cla=CLA,
            ins=first_ins,
            p1=P1.MORE_APDU,
            p2=P2.LAST_APDU,
            data=b"\x00" * 8,
        )
    assert e.value.status == Errors.SW_DATA_PARSING_FAIL
