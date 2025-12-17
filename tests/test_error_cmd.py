import pytest
from application_client.stellar_command_sender import CLA, P1, P2, Errors, InsType
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
